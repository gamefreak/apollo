/*
 * ALURE  OpenAL utility library
 * Copyright (c) 2009 by Chris Robinson.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/* Title: Automatic Playback */

#include "config.h"

#include "main.h"

#include <list>
#include <vector>

#ifdef HAVE_WINDOWS_H

typedef struct {
    ALuint (*func)(ALvoid*);
    ALvoid *ptr;
    HANDLE thread;
} ThreadInfo;

static DWORD CALLBACK StarterFunc(void *ptr)
{
    ThreadInfo *inf = (ThreadInfo*)ptr;
    ALint ret;

    ret = inf->func(inf->ptr);
    ExitThread((DWORD)ret);

    return (DWORD)ret;
}

static ThreadInfo *StartThread(ALuint (*func)(ALvoid*), ALvoid *ptr)
{
    DWORD dummy;
    ThreadInfo *inf = new ThreadInfo;
    inf->func = func;
    inf->ptr = ptr;

    inf->thread = CreateThread(NULL, 0, StarterFunc, inf, 0, &dummy);
    if(!inf->thread)
    {
        delete inf;
        return NULL;
    }

    return inf;
}

static ALuint StopThread(ThreadInfo *inf)
{
    WaitForSingleObject(inf->thread, INFINITE);
    CloseHandle(inf->thread);
    delete inf;

    return 0;
}

#else

typedef struct {
    ALuint (*func)(ALvoid*);
    ALvoid *ptr;
    pthread_t thread;
} ThreadInfo;

static void *StarterFunc(void *ptr)
{
    ThreadInfo *inf = (ThreadInfo*)ptr;
    void *ret = (void*)(inf->func(inf->ptr));
    return ret;
}

static ThreadInfo *StartThread(ALuint (*func)(ALvoid*), ALvoid *ptr)
{
    ThreadInfo *inf = new ThreadInfo;
    inf->func = func;
    inf->ptr = ptr;

    if(pthread_create(&inf->thread, NULL, StarterFunc, inf) != 0)
    {
        delete inf;
        return NULL;
    }

    return inf;
}

static ALuint StopThread(ThreadInfo *inf)
{
    pthread_join(inf->thread, NULL);
    delete inf;

    return 0;
}

#endif

struct AsyncPlayEntry {
	ALuint source;
	alureStream *stream;
	std::vector<ALuint> buffers;
	ALsizei loopcount;
	ALsizei maxloops;
	void (*eos_callback)(void*,ALuint);
	void *user_data;
	bool finished;
	bool paused;
	alureUInt64 base_time;
	alureUInt64 max_time;
	ALuint queued_time;
	ALuint stream_freq;
	ALenum stream_format;
	ALuint stream_align;

	AsyncPlayEntry() : source(0), stream(NULL), loopcount(0), maxloops(0),
	                   eos_callback(NULL), user_data(NULL), finished(false),
	                   paused(false), base_time(0), max_time(0), queued_time(0),
	                   stream_freq(0), stream_format(AL_NONE), stream_align(0)
	{ }
	AsyncPlayEntry(const AsyncPlayEntry &rhs)
	  : source(rhs.source), stream(rhs.stream), buffers(rhs.buffers),
	    loopcount(rhs.loopcount), maxloops(rhs.maxloops),
	    eos_callback(rhs.eos_callback), user_data(rhs.user_data),
	    finished(rhs.finished), paused(rhs.paused), base_time(rhs.base_time),
	    max_time(rhs.max_time), queued_time(rhs.queued_time),
	    stream_freq(rhs.stream_freq), stream_format(rhs.stream_format),
	    stream_align(rhs.stream_align)
	{ }

	ALenum Update(ALint *queued)
	{
		ALint processed, state;

		alGetSourcei(source, AL_SOURCE_STATE, &state);
		alGetSourcei(source, AL_BUFFERS_QUEUED, queued);
		alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
		while(processed > 0)
		{
			ALint size, channels, bits;
			ALuint buf;

			alSourceUnqueueBuffers(source, 1, &buf);
			processed--;
			(*queued)--;

			alGetBufferi(buf, AL_SIZE, &size);
			alGetBufferi(buf, AL_CHANNELS, &channels);
			alGetBufferi(buf, AL_BITS, &bits);
			size /= channels * bits / 8;
			queued_time -= size;
			base_time += size;
			base_time %= max_time;

			while(!finished)
			{
				ALuint got = stream->GetData(stream->dataChunk, stream->chunkLen);
				got -= got%stream_align;
				if(got > 0)
				{
					alBufferData(buf, stream_format, stream->dataChunk, got, stream_freq);
					alSourceQueueBuffers(source, 1, &buf);
					(*queued)++;

					alGetBufferi(buf, AL_SIZE, &size);
					alGetBufferi(buf, AL_CHANNELS, &channels);
					alGetBufferi(buf, AL_BITS, &bits);
					size /= channels * bits / 8;
					queued_time += size;
					if(loopcount == 0)
						max_time += size;
					break;
				}
				if(loopcount == maxloops)
				{
					finished = true;
					break;
				}
				if(maxloops != -1 || loopcount < 1)
					loopcount++;
				finished = !stream->Rewind();
			}
		}

		return state;
	}
};
static std::list<AsyncPlayEntry> AsyncPlayList;
static ThreadInfo *PlayThreadHandle;

ALfloat CurrentInterval = 0.0f;

ALuint AsyncPlayFunc(ALvoid*)
{
	EnterCriticalSection(&cs_StreamPlay);
	while(CurrentInterval > 0.0f)
	{
		alureUpdate();

		ALfloat interval = CurrentInterval;
		LeaveCriticalSection(&cs_StreamPlay);
		alureSleep(interval);
		EnterCriticalSection(&cs_StreamPlay);
	}
	LeaveCriticalSection(&cs_StreamPlay);
	return 0;
}


void StopStream(alureStream *stream)
{
	EnterCriticalSection(&cs_StreamPlay);

	std::list<AsyncPlayEntry>::iterator i = AsyncPlayList.begin(),
	                                    end = AsyncPlayList.end();
	while(i != end)
	{
		if(i->stream == stream)
		{
			AsyncPlayEntry ent(*i);
			AsyncPlayList.erase(i);

			alSourceStop(ent.source);
			alSourcei(ent.source, AL_BUFFER, 0);
			alDeleteBuffers(ent.buffers.size(), &ent.buffers[0]);
			alGetError();

			if(ent.eos_callback)
				ent.eos_callback(ent.user_data, ent.source);
			break;
		}
		i++;
	}

	LeaveCriticalSection(&cs_StreamPlay);
}


extern "C" {

/* Function: alurePlaySourceStream
 *
 * Starts playing a stream, using the specified source ID. A stream can only be
 * played if it is not already playing. You must call <alureUpdate> at regular
 * intervals to keep the stream playing, or else the stream will underrun and
 * cause a break in the playback until an update call can restart it. It also
 * is important that the current context is kept for <alureUpdate> calls,
 * otherwise the method may start calling OpenAL with invalid IDs. Note that
 * checking the state of the specified source is not a good method to determine
 * if a stream is playing. If an underrun occurs, the source will enter a
 * stopped state until it is automatically restarted. Instead, set a flag using
 * the callback to indicate the stream being stopped.
 *
 * Parameters:
 * source - The source ID to play the stream with. Any buffers on the source
 *          will be unqueued. It is valid to set source properties not related
 *          to the buffer queue or playback state (ie. you may change the
 *          source's position, pitch, gain, etc, but you must not stop the
 *          source or queue/unqueue buffers on it). To pause the source, call
 *          <alurePauseSource>.
 * stream - The stream to play. Any valid stream will work, although looping
 *          will only work if the stream can be rewound (eg. streams made with
 *          <alureCreateStreamFromCallback> cannot loop, but will play for as
 *          long as the callback provides data).
 * numBufs - The number of buffers used to queue with the OpenAL source. Each
 *           buffer will be filled with the chunk length specified when the
 *           stream was created. This value must be at least 2. More buffers at
 *           a larger size will decrease the time needed between updates, but
 *           at the cost of more memory usage.
 * loopcount - The number of times to loop the stream. When the stream reaches
 *             the end of processing, it will be rewound to continue buffering
 *             data. A value of -1 will cause the stream to loop indefinitely
 *             (or until <alureStopSource> is called).
 * eos_callback - This callback will be called when the stream reaches the end,
 *                no more loops are pending, and the source reaches a stopped
 *                state. It will also be called if an error occured and
 *                playback terminated.
 * userdata - An opaque user pointer passed to the callback.
 *
 * Returns:
 * AL_FALSE on error.
 *
 * *Version Added*: 1.1
 *
 * See Also:
 * <alureStopSource>, <alurePauseSource>, <alureGetSourceOffset>, <alureUpdate>
 */
ALURE_API ALboolean ALURE_APIENTRY alurePlaySourceStream(ALuint source,
    alureStream *stream, ALsizei numBufs, ALsizei loopcount,
    void (*eos_callback)(void *userdata, ALuint source), void *userdata)
{
	if(alGetError() != AL_NO_ERROR)
	{
		SetError("Existing OpenAL error");
		return AL_FALSE;
	}

	if(!alureStream::Verify(stream))
	{
		SetError("Invalid stream pointer");
		return AL_FALSE;
	}

	if(numBufs < 2)
	{
		SetError("Invalid buffer count");
		return AL_FALSE;
	}

	if(!alIsSource(source))
	{
		SetError("Invalid source ID");
		return AL_FALSE;
	}

	EnterCriticalSection(&cs_StreamPlay);

	std::list<AsyncPlayEntry>::iterator i = AsyncPlayList.begin(),
	                                    end = AsyncPlayList.end();
	while(i != end)
	{
		if(i->stream == stream)
		{
			SetError("Stream is already playing");
			LeaveCriticalSection(&cs_StreamPlay);
			return AL_FALSE;
		}
		if(i->source == source)
		{
			SetError("Source is already playing");
			LeaveCriticalSection(&cs_StreamPlay);
			return AL_FALSE;
		}
		i++;
	}

	AsyncPlayEntry ent;
	ent.stream = stream;
	ent.source = source;
	ent.maxloops = loopcount;
	ent.eos_callback = eos_callback;
	ent.user_data = userdata;

	ent.buffers.resize(numBufs);
	alGenBuffers(ent.buffers.size(), &ent.buffers[0]);
	if(alGetError() != AL_NO_ERROR)
	{
		LeaveCriticalSection(&cs_StreamPlay);
		SetError("Error generating buffers");
		return AL_FALSE;
	}

	numBufs = 0;
	if(ent.stream->GetFormat(&ent.stream_format, &ent.stream_freq, &ent.stream_align))
	{
		for(size_t i = 0;i < ent.buffers.size();i++)
		{
			ALuint got = ent.stream->GetData(ent.stream->dataChunk,
			                                 ent.stream->chunkLen);
			got -= got%ent.stream_align;
			if(got <= 0)
			{
				if(ent.loopcount == ent.maxloops || i == 0)
					ent.finished = true;
				else
				{
					if(ent.maxloops != -1 || ent.loopcount < 1)
						ent.loopcount++;
					ent.finished = !ent.stream->Rewind();
				}
				if(ent.finished)
					break;
				i--;
				continue;
			}
			ALuint buf = ent.buffers[i];
			alBufferData(buf, ent.stream_format, ent.stream->dataChunk, got, ent.stream_freq);
			numBufs++;

			ALint size, channels, bits;
			alGetBufferi(buf, AL_SIZE, &size);
			alGetBufferi(buf, AL_CHANNELS, &channels);
			alGetBufferi(buf, AL_BITS, &bits);
			size /= channels * bits / 8;
			ent.queued_time += size;
			if(ent.loopcount == 0)
				ent.max_time += size;
		}
	}
	if(numBufs == 0)
	{
		alDeleteBuffers(ent.buffers.size(), &ent.buffers[0]);
		alGetError();
		LeaveCriticalSection(&cs_StreamPlay);
		SetError("Error buffering from stream");
		return AL_FALSE;
	}

	if((alSourcei(source, AL_LOOPING, AL_FALSE),
	    alSourcei(source, AL_BUFFER, 0),alGetError()) != AL_NO_ERROR ||
	   (alSourceQueueBuffers(source, numBufs, &ent.buffers[0]),
	    alSourcePlay(source),alGetError()) != AL_NO_ERROR)
	{
		alSourcei(source, AL_BUFFER, 0);
		alDeleteBuffers(ent.buffers.size(), &ent.buffers[0]);
		alGetError();
		LeaveCriticalSection(&cs_StreamPlay);
		SetError("Error starting source");
		return AL_FALSE;
	}

	AsyncPlayList.push_front(ent);

	LeaveCriticalSection(&cs_StreamPlay);

	return AL_TRUE;
}

/* Function: alurePlaySource
 *
 * Plays the specified source ID and watches for it to stop. When a source
 * enters an AL_STOPPED state, the specified callback will be called by
 * <alureUpdate> to alert the application. As with <alurePlaySourceStream>, the
 * current context must not be changed while the source is being watched
 * (before the callback is called or <alureStopSource> is called). It also must
 * not be deleted while being watched.
 *
 * Parameters:
 * source - The source ID to play. As with <alurePlaySourceStream>, it is valid
 *          to set source properties not related to the playback state (ie. you
 *          may change a source's position, pitch, gain, etc). Pausing a source
 *          and restarting a paused source is allowed, and the callback will
 *          still be invoked when the source naturally reaches an AL_STOPPED
 *          state.
 * callback - The callback to be called when the source stops.
 * userdata - An opaque user pointer passed to the callback.
 *
 * Returns:
 * AL_FALSE on error.
 *
 * *Version Added*: 1.1
 *
 * See Also:
 * <alureStopSource>, <alureUpdate>
 */
ALURE_API ALboolean ALURE_APIENTRY alurePlaySource(ALuint source,
    void (*callback)(void *userdata, ALuint source), void *userdata)
{
	if(alGetError() != AL_NO_ERROR)
	{
		SetError("Existing OpenAL error");
		return AL_FALSE;
	}

	EnterCriticalSection(&cs_StreamPlay);

	std::list<AsyncPlayEntry>::iterator i = AsyncPlayList.begin(),
	                                    end = AsyncPlayList.end();
	while(i != end)
	{
		if(i->source == source)
		{
			SetError("Source is already playing");
			LeaveCriticalSection(&cs_StreamPlay);
			return AL_FALSE;
		}
		i++;
	}

	if((alSourcePlay(source),alGetError()) != AL_NO_ERROR)
	{
		LeaveCriticalSection(&cs_StreamPlay);
		SetError("Error starting source");
		return AL_FALSE;
	}

	if(callback != NULL)
	{
		AsyncPlayEntry ent;
		ent.source = source;
		ent.eos_callback = callback;
		ent.user_data = userdata;
		AsyncPlayList.push_front(ent);
	}

	LeaveCriticalSection(&cs_StreamPlay);

	return AL_TRUE;
}

/* Function: alureStopSource
 *
 * Stops the specified source ID, and any associated stream. The previously
 * specified callback will be invoked if 'run_callback' is not AL_FALSE.
 * Sources that were not started with <alurePlaySourceStream> or
 * <alurePlaySource> will still be stopped, but will not have any callback
 * called for them.
 *
 * Returns:
 * AL_FALSE on error.
 *
 * *Version Added*: 1.1
 *
 * See Also:
 * <alurePlaySourceStream>, <alurePlaySource>
 */
ALURE_API ALboolean ALURE_APIENTRY alureStopSource(ALuint source, ALboolean run_callback)
{
	if(alGetError() != AL_NO_ERROR)
	{
		SetError("Existing OpenAL error");
		return AL_FALSE;
	}

	EnterCriticalSection(&cs_StreamPlay);

	if((alSourceStop(source),alGetError()) != AL_NO_ERROR)
	{
		LeaveCriticalSection(&cs_StreamPlay);
		SetError("Error stopping source");
		return AL_FALSE;
	}

	std::list<AsyncPlayEntry>::iterator i = AsyncPlayList.begin(),
	                                    end = AsyncPlayList.end();
	while(i != end)
	{
		if(i->source == source)
		{
			AsyncPlayEntry ent(*i);
			AsyncPlayList.erase(i);

			if(ent.buffers.size() > 0)
			{
				alSourcei(ent.source, AL_BUFFER, 0);
				alDeleteBuffers(ent.buffers.size(), &ent.buffers[0]);
				alGetError();
			}

			if(run_callback && ent.eos_callback)
				ent.eos_callback(ent.user_data, ent.source);
			break;
		}
		i++;
	}

	LeaveCriticalSection(&cs_StreamPlay);

	return AL_TRUE;
}

/* Function: alurePauseSource
 *
 * Pauses the specified source ID, and any associated stream. This is needed to
 * avoid potential race conditions with sources that are playing a stream.
 *
 * Note that it is possible for the specified source to become stopped, and any
 * associated stream to finish, before this function is called, causing the
 * callback to be delayed until after the function returns and <alureUpdate>
 * detects the stopped source.
 *
 * Returns:
 * AL_FALSE on error.
 *
 * *Version Added*: 1.1
 *
 * See Also:
 * <alureResumeSource>, <alurePlaySourceStream>, <alurePlaySource>
 */
ALURE_API ALboolean ALURE_APIENTRY alurePauseSource(ALuint source)
{
	if(alGetError() != AL_NO_ERROR)
	{
		SetError("Existing OpenAL error");
		return AL_FALSE;
	}

	EnterCriticalSection(&cs_StreamPlay);

	if((alSourcePause(source),alGetError()) != AL_NO_ERROR)
	{
		SetError("Error pausing source");
		LeaveCriticalSection(&cs_StreamPlay);
		return AL_FALSE;
	}

	std::list<AsyncPlayEntry>::iterator i = AsyncPlayList.begin(),
	                                    end = AsyncPlayList.end();
	while(i != end)
	{
		if(i->source == source)
		{
			i->paused = true;
			break;
		}
		i++;
	}

	LeaveCriticalSection(&cs_StreamPlay);

	return AL_TRUE;
}

/* Function: alureResumeSource
 *
 * Resumes the specified source ID after being paused.
 *
 * Returns:
 * AL_FALSE on error.
 *
 * *Version Added*: 1.1
 *
 * See Also:
 * <alurePauseSource>
 */
ALURE_API ALboolean ALURE_APIENTRY alureResumeSource(ALuint source)
{
	if(alGetError() != AL_NO_ERROR)
	{
		SetError("Existing OpenAL error");
		return AL_FALSE;
	}

	EnterCriticalSection(&cs_StreamPlay);

	if((alSourcePlay(source),alGetError()) != AL_NO_ERROR)
	{
		SetError("Error playing source");
		LeaveCriticalSection(&cs_StreamPlay);
		return AL_FALSE;
	}

	std::list<AsyncPlayEntry>::iterator i = AsyncPlayList.begin(),
	                                    end = AsyncPlayList.end();
	while(i != end)
	{
		if(i->source == source)
		{
			i->paused = false;
			break;
		}
		i++;
	}

	LeaveCriticalSection(&cs_StreamPlay);

	return AL_TRUE;
}

/* Function: alureGetSourceOffset
 *
 * Gets the sample offset of the specified source. For sources started with
 * <alurePlaySourceStream>, this will be the total samples played. The offset
 * will loop back to 0 when the stream rewinds for any specified loopcount. For
 * non-streamed sources, the function will behave as if retrieving the
 * AL_SAMPLE_OFFSET source value.
 *
 * Returns:
 * (alureUInt64)-1 on error.
 *
 * *Version Added*: 1.1
 *
 * See Also:
 * <alurePlaySourceStream>
 */
ALURE_API alureUInt64 ALURE_APIENTRY alureGetSourceOffset(ALuint source)
{
	if(alGetError() != AL_NO_ERROR)
	{
		SetError("Existing OpenAL error");
		return (alureUInt64)-1;
	}

	EnterCriticalSection(&cs_StreamPlay);

	ALint pos;
	if((alGetSourcei(source, AL_SAMPLE_OFFSET, &pos),alGetError()) != AL_NO_ERROR)
	{
		LeaveCriticalSection(&cs_StreamPlay);
		SetError("Error retrieving source offset");
		return (alureUInt64)-1;
	}

	alureUInt64 retval = static_cast<ALuint>(pos);
	std::list<AsyncPlayEntry>::iterator i = AsyncPlayList.begin(),
	                                    end = AsyncPlayList.end();
	while(i != end)
	{
		if(i->source == source)
		{
			ALint state;

			alGetSourcei(source, AL_SOURCE_STATE, &state);

			retval = i->base_time + i->queued_time;
			if(state != AL_STOPPED)
				retval -= i->queued_time - pos;

			retval %= i->max_time;
			break;
		}
		i++;
	}

	LeaveCriticalSection(&cs_StreamPlay);

	return retval;
}

/* Function: alureUpdate
 *
 * Updates the running list of streams, and checks for stopped sources. This
 * makes sure that sources played with <alurePlaySourceStream> are kept fed
 * from their associated stream, and sources played with <alurePlaySource> are
 * still playing. It will call their callbacks as needed.
 *
 * *Version Added*: 1.1
 *
 * See Also:
 * <alurePlaySourceStream>, <alurePlaySource>
 */
ALURE_API void ALURE_APIENTRY alureUpdate(void)
{
	EnterCriticalSection(&cs_StreamPlay);
restart:
	std::list<AsyncPlayEntry>::iterator i = AsyncPlayList.begin(),
	                                    end = AsyncPlayList.end();
	for(;i != end;i++)
	{
		if(i->stream == NULL)
		{
			ALint state;
			alGetSourcei(i->source, AL_SOURCE_STATE, &state);
			if(state != AL_PLAYING && state != AL_PAUSED)
			{
				AsyncPlayEntry ent(*i);
				AsyncPlayList.erase(i);
				ent.eos_callback(ent.user_data, ent.source);
				goto restart;
			}
			continue;
		}

		ALint queued;
		if(i->Update(&queued) != AL_PLAYING)
		{
			if(queued == 0)
			{
				AsyncPlayEntry ent(*i);
				AsyncPlayList.erase(i);

				alSourcei(ent.source, AL_BUFFER, 0);
				alDeleteBuffers(ent.buffers.size(), &ent.buffers[0]);
				if(ent.eos_callback)
					ent.eos_callback(ent.user_data, ent.source);
				goto restart;
			}
			if(!i->paused)
				alSourcePlay(i->source);
		}
	}
	LeaveCriticalSection(&cs_StreamPlay);
}

/* Function: alureUpdateInterval
 *
 * Sets up a timer or thread to automatically call <alureUpdate> at the given
 * interval, in seconds. If the timer or thread is already running, the update
 * interval will be modified. A 0 or negative interval will stop <alureUpdate>
 * from being called.
 *
 * Returns:
 * AL_FALSE on error.
 *
 * *Version Added*: 1.1
 *
 * See Also:
 * <alureUpdate>
 */
ALURE_API ALboolean ALURE_APIENTRY alureUpdateInterval(ALfloat interval)
{
	EnterCriticalSection(&cs_StreamPlay);
	if(interval <= 0.0f)
	{
		CurrentInterval = 0.0f;
		if(PlayThreadHandle)
		{
			ThreadInfo *threadinf = PlayThreadHandle;
			PlayThreadHandle = NULL;
			LeaveCriticalSection(&cs_StreamPlay);
			StopThread(threadinf);
			EnterCriticalSection(&cs_StreamPlay);
		}
	}
	else
	{
		if(!PlayThreadHandle)
			PlayThreadHandle = StartThread(AsyncPlayFunc, NULL);
		if(!PlayThreadHandle)
		{
			SetError("Error starting async thread");
			LeaveCriticalSection(&cs_StreamPlay);
			return AL_FALSE;
		}
		CurrentInterval = interval;
	}
	LeaveCriticalSection(&cs_StreamPlay);

	return AL_TRUE;
}

} // extern "C"

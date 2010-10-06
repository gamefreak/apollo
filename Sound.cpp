#include <apollo/Sound/Sound.h>
#include <AL/alure.h>

#include <map>
#include <apollo/Utilities/ResourceManager.h>
#include <assert.h>
#include <apollo/Logging.h>

namespace Sound
{

const int CHUNK_LENGTH = 16386;

typedef std::map<std::string, ALuint> BufferMap;
BufferMap sounds;

int soundSourceCount;
ALuint* soundSources;
ALuint musicSource;
ALuint musicBufs[2];
alureStream* currentMusicStream = NULL;
std::string currentMusicName = "";
int soundSourceIndex = 0;

void Quit()
{
	alureUpdateInterval(0);
	StopMusic();
	alureSleep(0.1f);
	alureShutdownDevice();
}

ALubyte* GetSoundData(const std::string& path, size_t& length)
{
	SDL_RWops* rwops = ResourceManager::OpenFile(path + ".aiff");
	if (!rwops)
		rwops = ResourceManager::OpenFile(path + ".ogg");
	if (!rwops)
	{
		length = 0;
		return NULL;
	}
	return (ALubyte*)ResourceManager::ReadFull(&length, rwops, 1);
}

static ALuint GetSound(const std::string& name)
{
	BufferMap::iterator iter = sounds.find(name);
	if (iter != sounds.end())
		return iter->second;
	size_t length;
	ALubyte* data = GetSoundData("Sounds/" + name, length);
	ALuint buf = alureCreateBufferFromMemory(data, length);
	free(data);
	sounds.insert(std::make_pair(name, buf));
	return buf;
}

void Init(int frequency, int resolution, int sources)
{
	(void)resolution;
	--sources; // one spare source for music
	ALCint attribs[] = {
		ALC_FREQUENCY, frequency,
		/*ALC_MONO_SOURCES, sources,
		ALC_STEREO_SOURCES, 1,*/
		0
	};
	alureInitDevice(NULL, attribs);
	soundSources = new ALuint[sources];
	alGenSources(sources, soundSources);
	alGenSources(1, &musicSource);
	//alGenBuffers(2, musicBufs);
	soundSourceCount = sources;
	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
	alSpeedOfSound(1400.0);
	alDopplerFactor(0.7);
	alureUpdateInterval(0.03333f);
}

static ALuint GetFreeSource()
{
	int idx = soundSourceIndex++;
	soundSourceIndex = soundSourceIndex % soundSourceCount;
	return soundSources[idx];
}

void Preload(const std::string& name)
{
	GetSound(name);
}

void PlaySound(const std::string& name, float gain)
{
	const static ALfloat fz[] = { 0.0f, 0.0f, 0.0f };
	ALuint buf    = GetSound(name);
	ALuint source = GetFreeSource();
	LOG("Sound", LOG_NOTICE, "playing sound %s on channel %d", name.c_str(), source);
	alSourcei(source, AL_BUFFER, buf);
	alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
	alSourcef(source, AL_GAIN, gain);
	alSourcefv(source, AL_POSITION, fz);
	alSourcefv(source, AL_VELOCITY, fz);
	alSourcePlay(source);
}

void PlaySoundPositional(const std::string& name, vec2 pos, vec2 vel, float gain)
{
	ALfloat fpos[] = {pos.X(), pos.Y(), 0.0f};
	ALfloat fvel[] = {vel.X(), vel.Y(), 0.0f};
	ALuint buf    = GetSound(name);
	ALuint source = GetFreeSource();
	alSourcei(source, AL_BUFFER, buf);
	alSourcei(source, AL_SOURCE_RELATIVE, AL_FALSE);
	alSourcefv(source, AL_POSITION, fpos);
	alSourcefv(source, AL_VELOCITY, fvel);
	alSourcef(source, AL_REFERENCE_DISTANCE, 50.0);
	alSourcef(source, AL_GAIN, gain);
	alSourcePlay(source);
}

static ALfloat forientation[] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

void SetListener(vec2 pos, vec2 vel)
{
	ALfloat fpos[] = {pos.X(), pos.Y(), 0.0f};
	ALfloat fvel[] = {vel.X(), vel.Y(), 0.0f};
	alListenerfv(AL_POSITION, fpos);
	alListenerfv(AL_VELOCITY, fvel);
	if (vel.ModulusSquared() > 0.2f)
	{
		forientation[0] = fvel[0];
		forientation[1] = fvel[1];
	}
	alListenerfv(AL_ORIENTATION, forientation);
}

static void MusicEndCallback(void* ud, ALuint source)
{
	alureDestroyStream(currentMusicStream, 2, musicBufs);
	currentMusicStream = NULL;
	currentMusicName = "";
	free(ud);
}

void PlayMusic(const std::string& name)
{
	if (currentMusicStream)
		alureStopSource(musicSource, AL_TRUE);
	size_t length;
	ALubyte* data = GetSoundData("Music/" + name, length);
	currentMusicName = name;
	currentMusicStream = alureCreateStreamFromStaticMemory(data, length, CHUNK_LENGTH, 2, musicBufs);
	alurePlaySourceStream(musicSource, currentMusicStream, 2, 1, MusicEndCallback, data);
	LOG("Sound", LOG_MESSAGE, "Music: %s", name.c_str());
	if (!data)
		LOG("Sound", LOG_WARNING, "failed to find music");
}

void StopMusic()
{
	if (!currentMusicStream)
		return;
	alureStopSource(musicSource, AL_TRUE);
}

std::string MusicName()
{
	return "";
}

float MusicVolume()
{
	ALfloat rv;
	alGetSourcefv(musicSource, AL_GAIN, &rv);
	return rv;
}

void SetMusicVolume(float mvol)
{
	alSourcef(musicSource, AL_GAIN, mvol);
}

float SoundVolume()
{
	return 1.0f;
}

void SetSoundVolume(float mvol)
{
}

}

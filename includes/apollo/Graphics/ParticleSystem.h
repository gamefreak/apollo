#ifndef __apollo_graphics_particle_system_h
#define __apollo_graphics_particle_system_h

#include <apollo/Apollo.h>
#include <apollo/Graphics/Graphics.h>
#include <apollo/Utilities/Vec2.h>
#include <apollo/Utilities/Colour.h>
#include <apollo/Graphics/ImageLoader.h>

namespace Graphics
{

class ParticleSystem
{
private:
	vec2 acceleration;
	vec2* velocity;
	vec2* position;
	vec2 size;
	float sizeFactor;
	float baseTime, expireTime, lastTime;
	unsigned long count;
	GLuint texID;
public:
	ParticleSystem ( const std::string& name, unsigned long count, float sf, vec2 centre, vec2 averageVelocity, vec2 velocityVariance, vec2 accel, float lifetime );
	~ParticleSystem () { delete [] velocity; delete [] position; }
	
	void Draw ();
	// if this returns true, delete us
	bool Update ();
};

}

#endif

#ifndef __apollo_graphics_object3d_h
#define __apollo_graphics_object3d_h

#include <apollo/Graphics/opengl.h>
#include <SDL.h>
#include <SDL_image.h>
#include <apollo/Utilities/Vec2.h>
#include <string>
#include <vector>

namespace Graphics
{

class Object3D
{
private:
	GLuint vertexVBO;
	GLuint normalsVBO;
	GLuint texVBO;
	GLuint tangentVBO;
	GLuint texture;
	float offX, offY, intScale;
	float shininess, specScale;
	unsigned int nverts;
	void LoadObject(const std::string& name);
	void LoadTexture(const std::string& name);
public:
	Object3D ( const std::string& name );
	~Object3D ();
	
	void BindTextures ();
	void Draw ( float scale, float angle, float bank = 0.0f );
	float Shininess() const { return shininess; }
	float SpecularScale() const { return specScale; }
};

}

#endif

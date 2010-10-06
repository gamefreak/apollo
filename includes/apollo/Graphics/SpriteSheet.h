#ifndef __apollo_graphics_spritesheet_h
#define __apollo_graphics_spritesheet_h

#ifdef WIN32
#include <gl/gl.h>
#include <SDL/SDL_image.h>
#else
#include <OpenGL/gl.h>
#include <SDL/SDL_image.h>
#endif

#include <SDL/SDL.h>
#include <apollo/Utilities/Vec2.h>
#include <string>

namespace Graphics
{

class SpriteSheet
{
private:
	SDL_Surface* surface;
	GLuint texID;
	int sheetTilesX, sheetTilesY;
	int tileSizeX, tileSizeY;
	bool rotational;
	void MakeResident ();
public:
	float scaleFactor;
	SpriteSheet ( const std::string& name );
	~SpriteSheet ();
	
	bool IsRotational () const { return rotational; }
	float ScaleFactor () const { return scaleFactor; }
	
	int SheetTilesX () const { return sheetTilesX; }
	int SheetTilesY () const { return sheetTilesY; }
	
	int TileSizeX () const { return tileSizeX; }
	int TileSizeY () const { return tileSizeY; }
	
	void Draw ( int x, int y, const vec2& size );
	void DrawRotation ( const vec2& size, float angle ); // 0...2π from east
};

}

#endif

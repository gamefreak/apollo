#ifndef __apollo_graphics_text_renderer_h
#define __apollo_graphics_text_renderer_h

#include <string>
#ifdef WIN32
#include <gl/gl.h>
#else
#include <OpenGL/gl.h>
#endif
#include <apollo/Utilities/Vec2.h>

namespace Graphics
{

namespace TextRenderer
{

vec2 TextDimensions ( const std::string& font, const std::string& text, float size );
GLuint TextObject ( const std::string& font, const std::string& text, float size );
void Prune ();
void Flush ();

}

}

#endif

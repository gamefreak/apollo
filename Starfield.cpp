#include <apollo/Graphics/glu.h>

#include <SDL_image.h>
#include <apollo/Apollo.h>
#include <apollo/Graphics/Starfield.h>
#include <string>
#include <vector>
#include <TinyXML/tinyxml.h>
#include <apollo/Utilities/ResourceManager.h>
#include <apollo/Utilities/Matrix2x3.h>

namespace Graphics
{

namespace Matrices
{

void SetProjectionMatrix ( const matrix2x3& m );
void SetViewMatrix ( const matrix2x3& m );
void SetModelMatrix ( const matrix2x3& m );
const matrix2x3& CurrentMatrix ();
const matrix2x3& ProjectionMatrix ();
const matrix2x3& ViewMatrix ();
const matrix2x3& ModelMatrix ();

}

namespace StarfieldBuilding
{

std::vector<std::pair<SDL_Surface*, float> > starTypes;

void InitStars ()
{
	if (starTypes.empty())
	{
		// load the XML config file
		SDL_RWops* rwops = ResourceManager::OpenFile("Stars/Starfield.xml");
		assert(rwops);
		size_t len;
		char* ptr = (char*)ResourceManager::ReadFull(&len, rwops, 1);
		assert(ptr);
		char* fileData = (char*)malloc(len + 1);
		memcpy(fileData, ptr, len);
		fileData[len] = '\0';
		TiXmlDocument xmlDoc;
		xmlDoc.Parse(fileData);
		assert(!xmlDoc.Error());
		free((void*)fileData);
		TiXmlElement* root = xmlDoc.RootElement();
		assert(root);
		TiXmlElement* child = root->FirstChildElement("star");
		do
		{
			const char* starName = child->GetText();
			assert(starName);
			double starFrequency = 0.2f;
			child->Attribute("frequency", &starFrequency);
			SDL_RWops* rwops = ResourceManager::OpenFile(std::string("Stars/") + starName + ".png");
			assert(rwops);
			SDL_Surface* surface = IMG_Load_RW(rwops, 1);
			assert(surface);
			starTypes.push_back(std::make_pair(surface, (float)starFrequency));
		} while (child = child->NextSiblingElement());
	}
}

static SDL_Surface* surfaceTexture = NULL;

void CreateStarfieldTexture ()
{
	void* base = calloc(4, STARFIELD_WIDTH * STARFIELD_HEIGHT);
	surfaceTexture = SDL_CreateRGBSurfaceFrom(base, STARFIELD_WIDTH, STARFIELD_HEIGHT, 32, STARFIELD_WIDTH * 4, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
	uint8_t* components = (uint8_t*)surfaceTexture->pixels;
	for (int i = 0; i < STARFIELD_WIDTH * STARFIELD_HEIGHT; i++)
	{
		components[(i*4)+3] = 0x00;
	}
}

void UploadStarfieldTexture ()
{
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, STARFIELD_WIDTH, STARFIELD_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, surfaceTexture->pixels);
	SDL_FreeSurface(surfaceTexture);
	surfaceTexture = NULL;
}

void SpatterStars ( SDL_Surface* source, float freq );

void SpatterAllStars ( )
{
	for (std::vector<std::pair<SDL_Surface*, float> >::iterator iter = starTypes.begin(); iter != starTypes.end(); iter++)
	{
		SpatterStars(iter->first, iter->second);
	}
}

static const float starfieldSparseness = 16.0f;

void SpatterStars ( SDL_Surface* source, float freq )
{
	int count = (freq / starfieldSparseness) * (STARFIELD_WIDTH);
	count += (rand() % 3 - 1);
	if (count < 0) count = 0;
	int w_border = source->w / 2;
	int h_border = source->h / 2;
	int min_x = w_border;
	int max_x = STARFIELD_WIDTH - w_border;
	int min_y = h_border;
	int max_y = STARFIELD_HEIGHT - h_border;
	int x_w = max_x - min_x;
	int y_w = max_y - min_y;
	assert(w_border >= 0);
	assert(h_border >= 0);
	assert(min_x >= 0);
	assert(max_x >= 0);
	assert(min_y >= 0);
	assert(max_y >= 0);
	assert(x_w > 0);
	assert(y_w > 0);
	for (int i = 0; i < count; i++)
	{
		int x, y;
		x = (rand() % x_w) + min_x;
		y = (rand() % y_w) + min_y;
		assert(x >= 0);
		assert(y >= 0);
		assert(x < STARFIELD_WIDTH);
		assert(y < STARFIELD_HEIGHT);
		SDL_Rect dstRect;
		dstRect.x = (x - w_border);
		dstRect.y = (y - h_border);
		dstRect.w = w_border * 2;
		dstRect.h = h_border * 2;
		//glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, x - w_border, y - w_border, source->w, source->h, SDL_BYTEORDER == SDL_BIG_ENDIAN ? GL_RGBA : GL_ABGR_EXT, GL_UNSIGNED_BYTE, source->pixels);
		SDL_BlitSurface(source, NULL, surfaceTexture, &dstRect);
	}
}

}

using namespace StarfieldBuilding;

Starfield::Starfield ()
{
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	InitStars();
	CreateStarfieldTexture();
	SpatterAllStars();
	UploadStarfieldTexture();
}

Starfield::~Starfield ()
{
	glDeleteTextures(1, &texID);
}

vec2 Starfield::Dimensions ( float depth )
{
	(void)depth;
	return vec2(STARFIELD_WIDTH, STARFIELD_HEIGHT);
}

const static float starfieldScale = 1.98f;
const static float starfieldSpeed = 0.0003f;

void Starfield::Draw ( float depth, vec2 centre )
{
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	matrix2x3 oldProj = Matrices::ProjectionMatrix();
	Matrices::SetProjectionMatrix(matrix2x3());
	Matrices::SetViewMatrix(matrix2x3());
	Matrices::SetModelMatrix(matrix2x3());
	vec2 coordinates[4];
	coordinates[0] = vec2(-1.0f, -1.0f);
	coordinates[1] = vec2(1.0f,  -1.0f);
	coordinates[2] = vec2(1.0f,  1.0f );
	coordinates[3] = vec2(-1.0f, 1.0f );
	
	float depthTransform = depth + 1.0f;
	
	vec2 textureCoordinates[4];
	textureCoordinates[0] = (-centre*starfieldSpeed*depthTransform + vec2(-1.0, -1.0)) * starfieldScale;
	textureCoordinates[1] = (-centre*starfieldSpeed*depthTransform + vec2( 1.0, -1.0)) * starfieldScale;
	textureCoordinates[2] = (-centre*starfieldSpeed*depthTransform + vec2( 1.0,  1.0)) * starfieldScale;
	textureCoordinates[3] = (-centre*starfieldSpeed*depthTransform + vec2(-1.0,  1.0)) * starfieldScale;
	
	glVertexPointer(2, GL_FLOAT, 0, coordinates);
	glTexCoordPointer(2, GL_FLOAT, 0, textureCoordinates);
	glDrawArrays(GL_QUADS, 0, 4);
	
	Matrices::SetProjectionMatrix(oldProj);
	/*(void)depth;
	Matrices::SetViewMatrix(matrix2x3::Translate(centre));
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texID);
	
	const matrix2x3& mvpMatrix = Matrices::CurrentMatrix();
	matrix2x3 mvpInverse = mvpMatrix.Inverse();
	
	vec2 coordinates[4];
	coordinates[0] = mvpInverse * vec2(-1.0f, -1.0f);
	coordinates[1] = mvpInverse * vec2(1.0f,  -1.0f);
	coordinates[2] = mvpInverse * vec2(1.0f,  1.0f );
	coordinates[3] = mvpInverse * vec2(-1.0f, 1.0f );
	
	vec2 textureReferences[4];
	for (int i = 0; i < 4; i++)
	{
		textureReferences[i] = (coordinates[i] - centre) / starfieldScale;
	}
	
	glVertexPointer(2, GL_FLOAT, 0, coordinates);
	glTexCoordPointer(2, GL_FLOAT, 0, textureReferences);
	glDrawArrays(GL_QUADS, 0, 4);*/
}

}

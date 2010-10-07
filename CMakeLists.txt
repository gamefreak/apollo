PROJECT(Apollo)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
INCLUDE(FindSDL)
INCLUDE(FindSDL_image)
INCLUDE(FindSDL_ttf)
INCLUDE(FindLua51)
INCLUDE(FindOpenGL)

#FIND_PROGRAM(CLANG clang)
#FIND_PROGRAM(CLANGXX clang++)
#IF(NOT CLANG-NOTFOUND)
#SET(CMAKE_C_COMPILER clang)
#ENDIF()
#IF(NOT CLANGXX-NOTFOUND)
#SET(CMAKE_CXX_COMPILER clang++)
#ENDIF()

ADD_SUBDIRECTORY(external/eNet)
ADD_SUBDIRECTORY(external/alure)
ADD_SUBDIRECTORY(external/TinyXML)

SET(APOLLO_HEADERS
	./includes/apollo/Apollo.h
	./includes/apollo/Logging.h
	./includes/apollo/Utilities/Colour.h
	./includes/apollo/Utilities/Matrix2x3.h
	./includes/apollo/Utilities/Vec2.h
	./includes/apollo/Utilities/ResourceManager.h
	./includes/apollo/Utilities/GameTime.h
	./includes/apollo/Utilities/TestHarness.h
	./includes/apollo/Utilities/MachinePower.h
	./includes/apollo/Input.h
	./includes/apollo/Modes/ModeManager.h
	./includes/apollo/Scripting/Scripting.h
	./includes/apollo/Graphics/Graphics.h
	./includes/apollo/Net/MessageDecode.h
	./includes/apollo/Net/Net.h
	./includes/apollo/Scripting/Compile.h
	./includes/apollo/Sound/Sound.h
	./includes/apollo/Preferences.h
	./includes/apollo/Utilities/GAFFile.h
	./includes/apollo/Utilities/RNG.h
	./includes/apollo/Graphics/Shaders.h
	./includes/apollo/Graphics/ParticleSystem.h
	./includes/apollo/Graphics/SpriteSheet.h
	./includes/apollo/Graphics/TextRenderer.h
	./includes/apollo/Graphics/ImageLoader.h
	./includes/apollo/Graphics/Starfield.h
	./includes/apollo/Graphics/Object3D.h
	./includes/apollo/Graphics/opengl.h
	)

SET(APOLLO_SOURCE
	Entry.cpp
	Logging.cpp
	Colour.cpp
	Matrix2x3.cpp
	ResourceManager.cpp
	TestHarness.cpp
	MachinePower.cpp
	Input.cpp
	ModeManager.cpp
	Scripting.cpp
	Graphics.cpp
	Server.cpp
	Client.cpp
	MessageDecode.cpp
	Compile.cpp
	Sound.cpp
	Preferences.cpp
	GAFFile.cpp
	Shaders.cpp
	ParticleSystem.cpp
	SpriteSheet.cpp
	TextRenderer.cpp
	ImageLoader.cpp
	Starfield.cpp
	Object3D.cpp
	LuaBind.cpp
	)

INCLUDE_DIRECTORIES(external/alure/include)
INCLUDE_DIRECTORIES(includes)
INCLUDE_DIRECTORIES(external/eNet)
INCLUDE_DIRECTORIES(external)
INCLUDE_DIRECTORIES(
	${OPENGL_INCLUDE_DIR}
	${LUA_INCLUDE_DIR}
	${SDL_INCLUDE_DIR}
	${SDLIMAGE_INCLUDE_DIR}
	${SDLTTF_INCLUDE_DIR}
	)

ADD_LIBRARY(apollo STATIC ${APOLLO_HEADERS} ${APOLLO_SOURCE})

TARGET_LINK_LIBRARIES(apollo enet alure tinyxml
	${OPENGL_LIBRARIES}
	${LUA_LIBRARIES}
	${SDL_LIBRARY}
	${SDLIMAGE_LIBRARY}
	${SDLTTF_LIBRARY}
	${SDLMAIN_LIBRARY}
	)

MACRO(QTEST name)
	ADD_EXECUTABLE(${name} ./examples/${name})
	TARGET_LINK_LIBRARIES(${name} apollo)
ENDMACRO(QTEST)

QTEST(loading_example)

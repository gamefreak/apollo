#ifndef __apollo_h
#define __apollo_h

#ifdef WIN32
#define assert(x) { if (! ( x ) ) { printf("Assertion failure: %s\n\tFile: %s\n\tLine: %d\n", # x , __FILE__, __LINE__ ); abort(); } }
#undef SendMessage
#undef GetMessage	//apparently, these are windows directives
#endif

#include <assert.h>

#include <SDL.h>

#include "Logging.h"
#include "Utilities/Colour.h"
#include "Utilities/Matrix2x3.h"
#include "Utilities/Vec2.h"
#include "Utilities/ResourceManager.h"
#include "Utilities/GameTime.h"
#include "Utilities/TestHarness.h"
#include "Utilities/MachinePower.h"
#include "Input.h"
#include "Modes/ModeManager.h"
#include "Graphics/Graphics.h"
#include "Net/Net.h"
#include "Scripting/Scripting.h"
#include "Scripting/Compile.h"
#include "Sound/Sound.h"
#include "Preferences.h"
#include "eNetAdapt.h"

extern "C"
{

extern void Init ();
extern void MainLoop ();
extern void Shutdown ();

}

#endif

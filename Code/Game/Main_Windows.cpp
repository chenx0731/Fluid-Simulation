#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
App* g_theApp = nullptr;

#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#define UNUSED(x) (void)(x);
#include <windows.h>			// #include this (massive, platform-specific) header in very few places

//-----------------------------------------------------------------------------------------------
int WINAPI WinMain( HINSTANCE applicationInstanceHandle, HINSTANCE, LPSTR commandLineString, int )
{
	UNUSED(applicationInstanceHandle);
	UNUSED(commandLineString);
	
	g_theApp = new App();
	g_theApp->Startup();

	g_theApp->Run();
	
	g_theApp->Shutdown();
	delete g_theApp;
	g_theApp = nullptr;

	return 0;
}



#include "pch.h"

#include "tractor.h"

using namespace tractor;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/**
 * Main entry point.
 */
extern "C" int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
	Game* game = Game::getInstance();
	Platform* platform = Platform::create(game);
	assert(platform);
	int result = platform->enterMessagePump();
	delete platform;
	return result;
}

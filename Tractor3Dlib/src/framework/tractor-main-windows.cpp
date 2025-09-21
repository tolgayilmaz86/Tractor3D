#ifndef GP_NO_PLATFORM
#ifdef WIN32

#include "tractor.h"

using namespace tractor;

#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

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

#endif
#endif

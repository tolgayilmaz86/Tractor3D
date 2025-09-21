#pragma once

#include "SamplesGame.h"
#include "input/Gamepad.h"

using namespace tractor;

class GamepadSample : public Sample
{
public:

  GamepadSample();

  void gamepadEvent(Gamepad::GamepadEvent evt, Gamepad* gamepad);

protected:

  void initialize();

  void finalize();

  void update(float elapsedTime);

  void render(float elapsedTime);

private:

  void updateGamepad(float elapsedTime, Gamepad* gamepad, unsigned int player);

  const char* getStringFromButtonMapping(Gamepad::ButtonMapping mapping);

  Gamepad* _gamepad;
  Font* _font;
  std::string _status;
};

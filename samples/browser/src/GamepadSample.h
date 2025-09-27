#pragma once

#include "SamplesGame.h"

#include "input/Gamepad.h"

using namespace tractor;

class GamepadSample : public Sample
{
  public:
    GamepadSample() = default;

    void gamepadEvent(Gamepad::GamepadEvent evt, Gamepad* gamepad);

  protected:
    void initialize();

    void finalize();

    void update(float elapsedTime);

    void render(float elapsedTime);

  private:
    void updateGamepad(float elapsedTime, Gamepad* gamepad, unsigned int player);

    const std::string& getStringFromButtonMapping(Gamepad::ButtonMapping mapping);

    Gamepad* _gamepad{ nullptr };
    Font* _font{ nullptr };
    std::string _status{};
};

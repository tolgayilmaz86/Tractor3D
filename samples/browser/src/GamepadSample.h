/**
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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

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
#include "GamepadSample.h"

#if defined(ADD_SAMPLE)
ADD_SAMPLE("Input", "Gamepads", GamepadSample, 3);
#endif

static const std::unordered_map<Gamepad::ButtonMapping, std::string> buttonMap = {
    { Gamepad::BUTTON_A, std::string("A") },
    { Gamepad::BUTTON_B, std::string("B") },
    { Gamepad::BUTTON_X, std::string("X") },
    { Gamepad::BUTTON_Y, std::string("Y") },
    { Gamepad::BUTTON_L1, std::string("L1") },
    { Gamepad::BUTTON_L2, std::string("L2") },
    { Gamepad::BUTTON_L3, std::string("L3") },
    { Gamepad::BUTTON_R1, std::string("R1") },
    { Gamepad::BUTTON_R2, std::string("R2") },
    { Gamepad::BUTTON_R3, std::string("R3") },
    { Gamepad::BUTTON_UP, std::string("UP") },
    { Gamepad::BUTTON_DOWN, std::string("DOWN") },
    { Gamepad::BUTTON_LEFT, std::string("LEFT") },
    { Gamepad::BUTTON_RIGHT, std::string("RIGHT") },
    { Gamepad::BUTTON_MENU1, std::string("MENU1") },
    { Gamepad::BUTTON_MENU2, std::string("MENU2") },
    { Gamepad::BUTTON_MENU3, std::string("MENU3") }
};

void GamepadSample::initialize()
{
    setMultiTouch(true);

    _gamepad = getGamepad(0);
    // This is needed because the virtual gamepad is shared between all samples.
    // SamplesGame always ensures the virtual gamepad is disabled when a sample is exited.
    if (_gamepad && _gamepad->isVirtual()) _gamepad->getForm()->setEnabled(true);

    _font = Font::create("res/ui/arial.gpb");
    _status = "Looking for gamepads...";
}

void GamepadSample::finalize() { SAFE_RELEASE(_font); }

void GamepadSample::updateGamepad(float elapsedTime, Gamepad* gamepad, unsigned int player)
{
    std::ostringstream statusStream;
    statusStream << "Player: " << player << " - " << gamepad->getName() << "\nButtons: ";

    for (const auto& [button, strValue] : buttonMap)
        if (gamepad->isButtonDown(button))
            statusStream << strValue << " ";

    statusStream << "\n";

    // Joystick information
    for (size_t j = 0; j < gamepad->getJoystickCount(); ++j)
    {
        Vector2 joystick;
        gamepad->getJoystickValues(j, &joystick);
        statusStream << "Joystick " << j << ": (" << joystick.x << ", " << joystick.y << ")\n";
    }

    // Trigger information
    for (size_t k = 0; k < gamepad->getTriggerCount(); ++k)
    {
        statusStream << "Trigger " << k << ": " << gamepad->getTriggerValue(k) << "\n";
    }

    statusStream << "\n";
    _status = statusStream.str();
}

void GamepadSample::update(float elapsedTime)
{
    _status = "";
    if (_gamepad) updateGamepad(elapsedTime, _gamepad, 1);
}

void GamepadSample::render(float elapsedTime)
{
    clear(CLEAR_COLOR_DEPTH, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0);

    drawFrameRate(_font, Vector4(0, 0.5f, 1, 1), 5, 1, getFrameRate());

    _font->start();
    _font->drawText(_status, 7, 27, Vector4::one(), 22);
    _font->finish();

    _gamepad->draw();
}

void GamepadSample::gamepadEvent(Gamepad::GamepadEvent evt, Gamepad* gamepad)
{
    switch (evt)
    {
        case Gamepad::CONNECTED_EVENT:
        case Gamepad::DISCONNECTED_EVENT:
            _gamepad = getGamepad(0);
            // This is needed because the virtual gamepad is shared between all samples.
            // SamplesGame always ensures the virtual gamepad is disabled when a sample is exited.
            if (_gamepad && _gamepad->isVirtual()) _gamepad->getForm()->setEnabled(true);
            break;
    }
}

const std::string& GamepadSample::getStringFromButtonMapping(Gamepad::ButtonMapping mapping)
{
    static const std::unordered_map<Gamepad::ButtonMapping, std::string> buttonMap = {
        { Gamepad::BUTTON_A, std::string("A") },
        { Gamepad::BUTTON_B, std::string("B") },
        { Gamepad::BUTTON_X, std::string("X") },
        { Gamepad::BUTTON_Y, std::string("Y") },
        { Gamepad::BUTTON_L1, std::string("L1") },
        { Gamepad::BUTTON_L2, std::string("L2") },
        { Gamepad::BUTTON_L3, std::string("L3") },
        { Gamepad::BUTTON_R1, std::string("R1") },
        { Gamepad::BUTTON_R2, std::string("R2") },
        { Gamepad::BUTTON_R3, std::string("R3") },
        { Gamepad::BUTTON_UP, std::string("UP") },
        { Gamepad::BUTTON_DOWN, std::string("DOWN") },
        { Gamepad::BUTTON_LEFT, std::string("LEFT") },
        { Gamepad::BUTTON_RIGHT, std::string("RIGHT") },
        { Gamepad::BUTTON_MENU1, std::string("MENU1") },
        { Gamepad::BUTTON_MENU2, std::string("MENU2") },
        { Gamepad::BUTTON_MENU3, std::string("MENU3") }
    };
    auto it = buttonMap.find(mapping);
    return (it != buttonMap.end()) ? it->second : EMPTY_STRING;
}

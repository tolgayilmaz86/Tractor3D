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
#include "pch.h"

#include "input/Gamepad.h"

#include "framework/Game.h"
#include "framework/Platform.h"
#include "input/JoystickControl.h"
#include "ui/Button.h"
#include "ui/Form.h"

namespace tractor
{

static std::vector<Gamepad*> __gamepads;

Gamepad::Gamepad(const std::string& formPath) : _handle((GamepadHandle)INT_MAX)
{
    _form = Form::create(formPath);
    assert(_form);
    _form->setConsumeInputEvents(false);
    _name = "Virtual";

    for (size_t i = 0; i < 2; ++i)
    {
        _uiJoysticks[i] = nullptr;
        _triggers[i] = 0.0f;
    }

    for (size_t i = 0; i < 20; ++i)
    {
        _uiButtons[i] = nullptr;
    }

    bindGamepadControls(_form);
}

Gamepad::Gamepad(GamepadHandle handle,
                 unsigned int buttonCount,
                 unsigned int joystickCount,
                 unsigned int triggerCount,
                 const std::string& name)
    : _handle(handle), _buttonCount(buttonCount), _joystickCount(joystickCount),
      _triggerCount(triggerCount), _name(name)
{
    constexpr auto numberOfTriggers = 2;
    for (size_t i = 0; i < numberOfTriggers; ++i)
    {
        _triggers[i] = 0.0f;
    }
}

Gamepad::~Gamepad() { SAFE_RELEASE(_form); }

Gamepad* Gamepad::add(GamepadHandle handle,
                      unsigned int buttonCount,
                      unsigned int joystickCount,
                      unsigned int triggerCount,
                      const std::string& name)
{
    Gamepad* gamepad = new Gamepad(handle, buttonCount, joystickCount, triggerCount, name);

    __gamepads.push_back(gamepad);
    Game::getInstance()->gamepadEventInternal(CONNECTED_EVENT, gamepad);
    return gamepad;
}

Gamepad* Gamepad::add(const std::string& formPath)
{
    Gamepad* gamepad = new Gamepad(formPath);

    __gamepads.push_back(gamepad);
    Game::getInstance()->gamepadEventInternal(CONNECTED_EVENT, gamepad);
    return gamepad;
}

void Gamepad::remove(GamepadHandle handle)
{
    __gamepads.erase(std::remove_if(__gamepads.begin(),
                                    __gamepads.end(),
                                    [handle](Gamepad* gamepad)
                                    {
                                        if (gamepad->_handle == handle)
                                        {
                                            Game::getInstance()->gamepadEventInternal(DISCONNECTED_EVENT,
                                                                                      gamepad);
                                            SAFE_DELETE(gamepad);
                                            return true; // Mark for removal
                                        }
                                        return false; // Keep in container
                                    }),
                     __gamepads.end());
}

void Gamepad::remove(Gamepad* gamepad)
{
    __gamepads.erase(std::remove_if(__gamepads.begin(),
                                    __gamepads.end(),
                                    [&gamepad](Gamepad* g)
                                    {
                                        if (gamepad == g)
                                        {
                                            Game::getInstance()->gamepadEventInternal(DISCONNECTED_EVENT,
                                                                                      g);
                                            SAFE_DELETE(gamepad);
                                            return true; // Mark for removal
                                        }
                                        return false; // Keep in container
                                    }),
                     __gamepads.end());
}

void Gamepad::bindGamepadControls(Container* container)
{
    std::vector<Control*> controls = container->getControls();
    std::vector<Control*>::iterator itr = controls.begin();

    for (; itr != controls.end(); itr++)
    {
        Control* control = *itr;
        assert(control);

        if (control->isContainer())
        {
            bindGamepadControls((Container*)control);
        }
        else if (control->getTypeName() == "JoystickControl")
        {
            JoystickControl* joystick = (JoystickControl*)control;
            joystick->setConsumeInputEvents(true);
            _uiJoysticks[joystick->getIndex()] = joystick;
            _joystickCount++;
        }
        else if (control->getTypeName() == "Button")
        {
            Button* button = (Button*)control;
            button->setConsumeInputEvents(true);
            button->setCanFocus(false);
            _uiButtons[button->getDataBinding()] = button;
            _buttonCount++;
        }
    }
}

unsigned int Gamepad::getGamepadCount() { return __gamepads.size(); }

Gamepad* Gamepad::getGamepad(unsigned int index, bool preferPhysical)
{
    unsigned int count = __gamepads.size();
    if (index >= count) return nullptr;

    if (!preferPhysical) return __gamepads[index];

    // Virtual gamepads are guaranteed to come before physical gamepads in the vector.
    Gamepad* backupVirtual = nullptr;
    if (index < count && __gamepads[index]->isVirtual())
    {
        backupVirtual = __gamepads[index];
    }

    for (size_t i = 0; i < count; ++i)
    {
        if (!__gamepads[i]->isVirtual())
        {
            // __gamepads[i] is the first physical gamepad
            // and should be returned from getGamepad(0, true).
            if (index + i < count)
            {
                return __gamepads[index + i];
            }
        }
    }

    return backupVirtual;
}

Gamepad* Gamepad::getGamepad(GamepadHandle handle)
{
    size_t count = __gamepads.size();
    for (size_t i = 0; i < count; ++i)
    {
        if (__gamepads[i]->_handle == handle)
        {
            return __gamepads[i];
        }
    }
    return nullptr;
}

Gamepad::ButtonMapping Gamepad::getButtonMappingFromString(const std::string& string)
{
    if (string == "A" || string == "BUTTON_A")
        return BUTTON_A;
    else if (string == "B" || string == "BUTTON_B")
        return BUTTON_B;
    else if (string == "X" || string == "BUTTON_X")
        return BUTTON_X;
    else if (string == "Y" || string == "BUTTON_Y")
        return BUTTON_Y;
    else if (string == "L1" || string == "BUTTON_L1")
        return BUTTON_L1;
    else if (string == "L2" || string == "BUTTON_L2")
        return BUTTON_L2;
    else if (string == "L3" || string == "BUTTON_L3")
        return BUTTON_L3;
    else if (string == "R1" || string == "BUTTON_R1")
        return BUTTON_R1;
    else if (string == "R2" || string == "BUTTON_R2")
        return BUTTON_R2;
    else if (string == "R3" || string == "BUTTON_R3")
        return BUTTON_R3;
    else if (string == "UP" || string == "BUTTON_UP")
        return BUTTON_UP;
    else if (string == "DOWN" || string == "BUTTON_DOWN")
        return BUTTON_DOWN;
    else if (string == "LEFT" || string == "BUTTON_LEFT")
        return BUTTON_LEFT;
    else if (string == "RIGHT" || string == "BUTTON_RIGHT")
        return BUTTON_RIGHT;
    else if (string == "MENU1" || string == "BUTTON_MENU1")
        return BUTTON_MENU1;
    else if (string == "MENU2" || string == "BUTTON_MENU2")
        return BUTTON_MENU2;
    else if (string == "MENU3" || string == "BUTTON_MENU3")
        return BUTTON_MENU3;

    GP_WARN("Unknown string for ButtonMapping.");
    return BUTTON_A;
}

const std::string& Gamepad::getName() const { return _name; }

void Gamepad::update(float elapsedTime)
{
    if (!_form)
    {
        Platform::pollGamepadState(this);
    }
}

void Gamepad::updateInternal(float elapsedTime)
{
    size_t size = __gamepads.size();
    for (size_t i = 0; i < size; ++i)
    {
        __gamepads[i]->update(elapsedTime);
    }
}

void Gamepad::draw()
{
    if (_form && _form->isEnabled())
    {
        _form->draw();
    }
}

unsigned int Gamepad::getButtonCount() const { return _buttonCount; }

bool Gamepad::isButtonDown(ButtonMapping mapping) const
{
    if (_form)
    {
        Button* button = _uiButtons[mapping];
        if (button) return (button->getState() & Control::ACTIVE) != 0;
        return false;
    }
    else if (_buttons & (1 << mapping))
        return true;

    return false;
}

void Gamepad::getJoystickValues(unsigned int joystickId, Vector2* outValue) const
{
    if (joystickId >= _joystickCount) return;

    if (_form)
    {
        JoystickControl* joystick = _uiJoysticks[joystickId];
        if (joystick)
        {
            const Vector2& value = joystick->getValue();
            outValue->set(value.x, value.y);
        }
        else
        {
            outValue->set(0.0f, 0.0f);
        }
    }
    else
    {
        outValue->set(_joysticks[joystickId]);
    }
}

unsigned int Gamepad::getTriggerCount() const { return _triggerCount; }

float Gamepad::getTriggerValue(unsigned int triggerId) const
{
    if (triggerId >= _triggerCount) return 0.0f;

    // Triggers are not part of the virtual gamepad defintion
    if (_form)
        return 0.0f;
    else
        return _triggers[triggerId];
}

void Gamepad::setButtons(unsigned int buttons)
{
    if (buttons != _buttons)
    {
        _buttons = buttons;
        Form::gamepadButtonEventInternal(this);
    }
}

void Gamepad::setJoystickValue(unsigned int index, float x, float y)
{
    if (_joysticks[index].x != x || _joysticks[index].y != y)
    {
        _joysticks[index].set(x, y);
        Form::gamepadJoystickEventInternal(this, index);
    }
}

void Gamepad::setTriggerValue(unsigned int index, float value)
{
    if (_triggers[index] != value)
    {
        _triggers[index] = value;
        Form::gamepadTriggerEventInternal(this, index);
    }
}

} // namespace tractor

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

#include "framework/Platform.h"

#include "framework/Game.h"
#include "scripting/ScriptController.h"
#include "ui/Form.h"

namespace tractor
{

//----------------------------------------------------------------------------
void Platform::touchEventInternal(Touch::TouchEvent evt,
                                  int x,
                                  int y,
                                  unsigned int contactIndex,
                                  bool actuallyMouse)
{
    if (actuallyMouse || !Form::touchEventInternal(evt, x, y, contactIndex))
    {
        Game::getInstance()->touchEventInternal(evt, x, y, contactIndex);
    }
}

//----------------------------------------------------------------------------
void Platform::keyEventInternal(Keyboard::KeyEvent evt, int key)
{
    if (!Form::keyEventInternal(evt, key))
    {
        Game::getInstance()->keyEventInternal(evt, key);
    }
}

//----------------------------------------------------------------------------
bool Platform::mouseEventInternal(Mouse::MouseEvent evt, int x, int y, int wheelDelta)
{
    if (Form::mouseEventInternal(evt, x, y, wheelDelta)) return true;

    return Game::getInstance()->mouseEventInternal(evt, x, y, wheelDelta);
}

//----------------------------------------------------------------------------
void Platform::gestureSwipeEventInternal(int x, int y, int direction)
{
    Game::getInstance()->gestureSwipeEventInternal(x, y, direction);
}

//----------------------------------------------------------------------------
void Platform::gesturePinchEventInternal(int x, int y, float scale)
{
    Game::getInstance()->gesturePinchEventInternal(x, y, scale);
}

//----------------------------------------------------------------------------
void Platform::gestureTapEventInternal(int x, int y)
{
    Game::getInstance()->gestureTapEventInternal(x, y);
}

//----------------------------------------------------------------------------
void Platform::gestureLongTapEventInternal(int x, int y, float duration)
{
    Game::getInstance()->gestureLongTapEventInternal(x, y, duration);
}

//----------------------------------------------------------------------------
void Platform::gestureDragEventInternal(int x, int y)
{
    Game::getInstance()->gestureDragEventInternal(x, y);
}

//----------------------------------------------------------------------------
void Platform::gestureDropEventInternal(int x, int y)
{
    Game::getInstance()->gestureDropEventInternal(x, y);
}

//----------------------------------------------------------------------------
void Platform::resizeEventInternal(unsigned int width, unsigned int height)
{
    Game::getInstance()->resizeEventInternal(width, height);
    Form::resizeEventInternal(width, height);
}

//----------------------------------------------------------------------------
void Platform::gamepadEventConnectedInternal(GamepadHandle handle,
                                             unsigned int buttonCount,
                                             unsigned int joystickCount,
                                             unsigned int triggerCount,
                                             const char* name)
{
    Gamepad::add(handle, buttonCount, joystickCount, triggerCount, name);
}

//----------------------------------------------------------------------------
void Platform::gamepadEventDisconnectedInternal(GamepadHandle handle) { Gamepad::remove(handle); }

//----------------------------------------------------------------------------
void Platform::gamepadButtonPressedEventInternal(GamepadHandle handle, Gamepad::ButtonMapping mapping)
{
    Gamepad* gamepad = Gamepad::getGamepad(handle);
    if (gamepad)
    {
        unsigned int newButtons = gamepad->_buttons | (1 << mapping);
        gamepad->setButtons(newButtons);
        Form::gamepadButtonEventInternal(gamepad);
    }
}

//----------------------------------------------------------------------------
void Platform::gamepadButtonReleasedEventInternal(GamepadHandle handle, Gamepad::ButtonMapping mapping)
{
    Gamepad* gamepad = Gamepad::getGamepad(handle);
    if (gamepad)
    {
        unsigned int newButtons = gamepad->_buttons & ~(1 << mapping);
        gamepad->setButtons(newButtons);
        Form::gamepadButtonEventInternal(gamepad);
    }
}

//----------------------------------------------------------------------------
void Platform::gamepadTriggerChangedEventInternal(GamepadHandle handle, unsigned int index, float value)
{
    Gamepad* gamepad = Gamepad::getGamepad(handle);
    if (gamepad)
    {
        gamepad->setTriggerValue(index, value);
        Form::gamepadTriggerEventInternal(gamepad, index);
    }
}

//----------------------------------------------------------------------------
void Platform::gamepadJoystickChangedEventInternal(GamepadHandle handle,
                                                   unsigned int index,
                                                   float x,
                                                   float y)
{
    Gamepad* gamepad = Gamepad::getGamepad(handle);
    if (gamepad)
    {
        gamepad->setJoystickValue(index, x, y);
        Form::gamepadJoystickEventInternal(gamepad, index);
    }
}

} // namespace tractor

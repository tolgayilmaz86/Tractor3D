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
#include "InputSample.h"

#include "SamplesGame.h"

#if defined(ADD_SAMPLE)
ADD_SAMPLE("Input", "Basic Input", InputSample, 1);
#endif

/**
 * Returns the string representation of the given key.
 */
static const std::string& keyString(int key);

InputSample::InputSample()
    : _mouseString("No Mouse"), _font(nullptr), _inputSampleControls(nullptr), _mouseWheel(0),
      _crosshair(nullptr), _scene(nullptr), _formNode(nullptr), _formNodeParent(nullptr)
{
}

void InputSample::initialize()
{
    setMultiTouch(true);

    // Load font
    _font = Font::create("res/ui/arial.gpb");
    assert(_font);

    // Reuse part of the gamepad texture as the crosshair in this sample.
    _crosshair = SpriteBatch::create("res/png/gamepad.png");
    _crosshairDstRect.set(0, 0, 256, 256);
    _crosshairSrcRect.set(256, 0, 256, 256);
    _crosshairLowerLimit.set(-_crosshairSrcRect.width / 2.0f, -_crosshairSrcRect.height / 2.0f);
    _crosshairUpperLimit.set((float)getWidth(), (float)getHeight());
    _crosshairUpperLimit += _crosshairLowerLimit;

    // Create input sample controls
    _keyboardState = false;
    _inputSampleControls = Form::create("res/common/inputs.form");
    static_cast<Button*>(_inputSampleControls->getControl("showKeyboardButton"))
        ->addListener(this, Listener::CLICK);
    static_cast<Button*>(_inputSampleControls->getControl("captureMouseButton"))
        ->addListener(this, Listener::CLICK);

    if (!hasMouse())
    {
        static_cast<Button*>(_inputSampleControls->getControl("captureMouseButton"))->setVisible(false);
    }
    _inputSampleControls->getControl("restoreMouseLabel")->setVisible(false);

    _mousePoint.set(-100, -100);

    // Create a 3D form that responds to raw sensor inputs.
    // For this, we will need a scene with a camera node.
    Camera* camera =
        Camera::createPerspective(45.0f, (float)getWidth() / (float)getHeight(), 0.25f, 100.0f);
    _scene = Scene::create();
    Node* cameraNode = _scene->addNode("Camera");
    cameraNode->setCamera(camera);
    _scene->setActiveCamera(camera);
    SAFE_RELEASE(camera);
    _formNodeParent = _scene->addNode("FormParent");
    _formNode = Node::create("Form");
    _formNodeParent->addChild(_formNode);
    Theme* theme = _inputSampleControls->getTheme();
    Form* form = Form::create("testForm", theme->getStyle("basicContainer"), Layout::LAYOUT_ABSOLUTE);
    form->setSize(225, 100);
    Label* label = Label::create("sensorLabel", theme->getStyle("iconNoBorder"));
    label->setPosition(25, 15);
    label->setSize(175, 50);
    label->setText("Raw sensor response (accel/gyro)");
    form->addControl(label);
    label->release();
    _formNode->setScale(0.0015f, 0.0015f, 1.0f);
    _formNodeRestPosition.set(0, 0, -1.5f);
    _formNodeParent->setTranslation(_formNodeRestPosition);
    _formNode->setTranslation(-0.2f, -0.2f, 0);
    _formNode->setDrawable(form);
    form->release();
}

void InputSample::finalize()
{
    setMouseCaptured(false);
    if (_keyboardState)
    {
        displayKeyboard(false);
    }

    SAFE_RELEASE(_scene);
    SAFE_RELEASE(_formNode);
    SAFE_RELEASE(_inputSampleControls);
    SAFE_DELETE(_crosshair);
    SAFE_RELEASE(_font);
}

void InputSample::update(float elapsedTime)
{
    if (hasAccelerometer())
    {
        Vector3 accelRaw, gyroRaw;
        getSensorValues(&accelRaw.x, &accelRaw.y, &accelRaw.z, &gyroRaw.x, &gyroRaw.y, &gyroRaw.z);

        // Adjust for landscape mode
        float temp = accelRaw.x;
        accelRaw.x = -accelRaw.y;
        accelRaw.y = temp;
        temp = gyroRaw.x;
        gyroRaw.x = -gyroRaw.y;
        gyroRaw.y = temp;

        // Respond to raw accelerometer inputs
        Vector3 position;
        _formNodeParent->getTranslation(&position);
        position.smooth(_formNodeRestPosition - accelRaw * 0.04f, elapsedTime, 100);
        _formNodeParent->setTranslation(position);

        // Respond to raw gyroscope inputs
        Vector3 rotation;
        float angle = _formNodeParent->getRotation(&rotation);
        rotation *= angle;
        rotation.smooth(gyroRaw * (-0.18f), elapsedTime, 220);
        angle = rotation.length();
        rotation.normalize();
        _formNodeParent->setRotation(rotation, angle);
    }
}

void InputSample::render(float elapsedTime)
{
    // Clear the color and depth buffers
    clear(CLEAR_COLOR_DEPTH, Vector4::zero(), 1.0f, 0);

    _inputSampleControls->draw();

    // Draw text
    unsigned int fontSize = 18;
    Vector4 fontColor(1.0f, 1.0f, 1.0f, 1.0f);
    unsigned int width, height;
    char buffer[50];

    _font->start();

    if (isMouseCaptured())
    {
        // Draw crosshair at current offest w.r.t. center of screen
        _crosshair->start();
        _crosshair->draw(_crosshairDstRect, _crosshairSrcRect);
        _crosshair->finish();
    }
    else
    {
        for (const auto& tp : _touchPoints)
        {
            sprintf(buffer, "T_%u(%d,%d)", tp._id, (int)tp._coord.x, (int)tp._coord.y);
            _font->measureText(buffer, fontSize, &width, &height);
            int x = tp._coord.x - (int)(width >> 1);
            int y = tp._coord.y - (int)(height >> 1);
            _font->drawText(buffer, x, y, fontColor, fontSize);
        }

        // Mouse
        sprintf(buffer, "M(%d,%d)", (int)_mousePoint.x, (int)_mousePoint.y);
        _font->measureText(buffer, fontSize, &width, &height);
        int x = _mousePoint.x - (int)(width >> 1);
        int y = _mousePoint.y - (int)(height >> 1);
        _font->drawText(buffer, x, y, fontColor, fontSize);
        if (!_keyboardState && _mouseString.length() > 0)
        {
            int y = getHeight() - fontSize;
            _font->drawText(_mouseString, 0, y, fontColor, fontSize);
        }
        if (_mouseWheel)
        {
            sprintf(buffer, "%d", _mouseWheel);
            _font->measureText(buffer, fontSize, &width, &height);
            int x = _mouseWheelPoint.x - (int)(width >> 1);
            int y = _mouseWheelPoint.y + 4;
            _font->drawText(buffer, x, y, fontColor, fontSize);
        }
    }

    // Pressed keys
    if (_keyboardString.length() > 0)
    {
        _font->drawText(_keyboardString, 0, 0, fontColor, fontSize);
    }

    // Printable symbols typed
    if (_symbolsString.length() > 0)
    {
        _font->drawText(_symbolsString, 0, 18, fontColor, fontSize);
    }

    // Held keys
    if (!_downKeys.empty())
    {
        std::string displayKeys;
        for (std::set<int>::const_iterator i = _downKeys.begin(); i != _downKeys.end(); ++i)
        {
            const auto& str = keyString(*i);
            displayKeys.append(str);
        }
        if (!displayKeys.empty())
        {
            _font->measureText(displayKeys, 18, &width, &height);
            int x = Game::getInstance()->getWidth() - width;
            int y = 0;
            _font->drawText(displayKeys, x, y, fontColor, fontSize);
        }
    }

    // Draw the accelerometer values in the bottom right corner.
    static float pitch, roll;
    static float accelerometerDrawRate = 1000.0f;
    accelerometerDrawRate += elapsedTime;
    if (accelerometerDrawRate > 100.0f)
    {
        accelerometerDrawRate = 0.0f;
        getAccelerometerValues(&pitch, &roll);
    }
    if (hasAccelerometer() && !_keyboardState)
    {
        _formNode->getDrawable()->draw();

        sprintf(buffer, "Pitch: %f   Roll: %f", pitch, roll);
        _font->measureText(buffer, 18, &width, &height);
        _font->drawText(buffer, getWidth() - width, getHeight() - height, fontColor, fontSize);
    }
    _font->finish();
}

bool InputSample::drawScene(Node* node)
{
    Drawable* drawable = node->getDrawable();
    if (drawable) drawable->draw();
    return true;
}

void InputSample::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex)
{
    TouchPoint* tp = nullptr;
    // Not optimal, however we expect the list size to be very small (<10)
    for (std::list<TouchPoint>::iterator it = _touchPoints.begin(); it != _touchPoints.end(); ++it)
    {
        if (it->_id == contactIndex)
        {
            tp = &(*it); // (seems redundant, however due to STD)
            break;
        }
    }

    // Add a new touch point if not found above
    if (!tp)
    {
        tp = new TouchPoint();
        tp->_id = contactIndex;
        _touchPoints.push_back(*tp);
    }

    // Update the touch point
    tp->_coord.x = x;
    tp->_coord.y = y;
    tp->_isStale = false; // (could be overwritten below)

    switch (evt)
    {
        case Touch::TOUCH_PRESS:
            // Purge all stale touch points
            for (std::list<TouchPoint>::iterator it = _touchPoints.begin(); it != _touchPoints.end();)
            {
                if (it->_isStale)
                {
                    it = _touchPoints.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            if (x < 30 && y < 30)
            {
                displayKeyboard(true);
            }
            break;
        case Touch::TOUCH_RELEASE:
            // Mark the current touch point as stale
            if (tp)
            {
                tp->_isStale = true;
            }
            break;
        case Touch::TOUCH_MOVE:
            break;
    };
}

bool InputSample::mouseEvent(Mouse::MouseEvent evt, int x, int y, int wheelDelta)
{
    _mousePoint.set(x, y);
    _mouseString.clear();

    switch (evt)
    {
        case Mouse::MOUSE_PRESS_LEFT_BUTTON:
            _mouseString.append("MOUSE_PRESS_LEFT_BUTTON");
            break;
        case Mouse::MOUSE_RELEASE_LEFT_BUTTON:
            _mouseString.append("MOUSE_RELEASE_LEFT_BUTTON");
            break;
        case Mouse::MOUSE_PRESS_MIDDLE_BUTTON:
            _mouseString.append("MOUSE_PRESS_MIDDLE_BUTTON");
            break;
        case Mouse::MOUSE_RELEASE_MIDDLE_BUTTON:
            _mouseString.append("MOUSE_RELEASE_MIDDLE_BUTTON");
            break;
        case Mouse::MOUSE_PRESS_RIGHT_BUTTON:
            _mouseString.append("MOUSE_PRESS_RIGHT_BUTTON");
            break;
        case Mouse::MOUSE_RELEASE_RIGHT_BUTTON:
            _mouseString.append("MOUSE_RELEASE_RIGHT_BUTTON");
            break;
        case Mouse::MOUSE_MOVE:
            _mouseString.append("MOUSE_MOVE");
            if (isMouseCaptured())
            {
                // Control crosshair from captured mouse
                _crosshairDstRect.setPosition(_crosshairDstRect.x + x, _crosshairDstRect.y + y);

                // Use screen limits to clamp the crosshair position
                Vector2 pos(_crosshairDstRect.x, _crosshairDstRect.y);
                pos.clamp(_crosshairLowerLimit, _crosshairUpperLimit);
                _crosshairDstRect.setPosition(pos.x, pos.y);
            }
            break;
        case Mouse::MOUSE_WHEEL:
            _mouseString.append("MOUSE_WHEEL");
            _mouseWheelPoint.x = x;
            _mouseWheelPoint.y = y;
            _mouseWheel = wheelDelta;
            break;
    }
    return false;
}

void InputSample::keyEvent(Keyboard::KeyEvent evt, int key)
{
    switch (evt)
    {
        case Keyboard::KEY_PRESS:
            _keyboardString.clear();
            _keyboardString.append(keyString(key));
            _keyboardString.append(" pressed");
            _downKeys.insert(key);

            if (key == Keyboard::KEY_ESCAPE)
            {
                _symbolsString.clear();
            }

            if (key == Keyboard::KEY_SPACE && hasMouse())
            {
                setCaptured(false);
            }
            break;
        case Keyboard::KEY_RELEASE:
            _keyboardString.clear();
            _keyboardString.append(keyString(key));
            _keyboardString.append(" released");
            _downKeys.erase(key);
            break;
        case Keyboard::KEY_CHAR:
            if (key == Keyboard::KEY_BACKSPACE)
            {
                if (_symbolsString.size() > 0) _symbolsString.resize((_symbolsString.size() - 1));
            }
            else
            {
                _symbolsString.append(1, (char)(0xFF & key));
            }
            break;
    };
}

void InputSample::controlEvent(Control* control, EventType evt)
{
    if (control->getId() == "showKeyboardButton")
    {
        _keyboardState = !_keyboardState;
        displayKeyboard(_keyboardState);
        static_cast<Button*>(_inputSampleControls->getControl("showKeyboardButton"))
            ->setText(_keyboardState ? "Hide virtual keyboard" : "Show virtual keyboard");
    }
    else if (control->getId() == "captureMouseButton" && hasMouse())
    {
        setCaptured(true);
    }
}

void InputSample::setCaptured(bool captured)
{
    setMouseCaptured(captured);

    if (!captured || isMouseCaptured())
    {
        _inputSampleControls->getControl("showKeyboardButton")->setVisible(!captured);
        _inputSampleControls->getControl("captureMouseButton")->setVisible(!captured);
        _inputSampleControls->getControl("restoreMouseLabel")->setVisible(captured);
    }

    if (captured)
    {
        _crosshairDstRect.setPosition((float)getWidth() / 2.0f + _crosshairLowerLimit.x,
                                      (float)getHeight() / 2.0f + _crosshairLowerLimit.y);
    }
}

const std::string& keyString(int key)
{
    static const std::unordered_map<int, std::string> keyMap = {
        { Keyboard::KEY_NONE, std::string("NONE") },
        { Keyboard::KEY_PAUSE, std::string("PAUSE") },
        { Keyboard::KEY_SCROLL_LOCK, std::string("SCROLL_LOCK") },
        { Keyboard::KEY_PRINT, std::string("PRINT") },
        { Keyboard::KEY_SYSREQ, std::string("SYSREQ") },
        { Keyboard::KEY_BREAK, std::string("BREAK") },
        { Keyboard::KEY_ESCAPE, std::string("ESCAPE") },
        { Keyboard::KEY_BACKSPACE, std::string("BACKSPACE") },
        { Keyboard::KEY_TAB, std::string("TAB") },
        { Keyboard::KEY_BACK_TAB, std::string("BACK_TAB") },
        { Keyboard::KEY_RETURN, std::string("RETURN") },
        { Keyboard::KEY_CAPS_LOCK, std::string("CAPS_LOCK") },
        { Keyboard::KEY_SHIFT, std::string("SHIFT") },
        { Keyboard::KEY_CTRL, std::string("CTRL") },
        { Keyboard::KEY_ALT, std::string("ALT") },
        { Keyboard::KEY_MENU, std::string("MENU") },
        { Keyboard::KEY_HYPER, std::string("HYPER") },
        { Keyboard::KEY_INSERT, std::string("INSERT") },
        { Keyboard::KEY_HOME, std::string("HOME") },
        { Keyboard::KEY_PG_UP, std::string("PG_UP") },
        { Keyboard::KEY_DELETE, std::string("DELETE") },
        { Keyboard::KEY_END, std::string("END") },
        { Keyboard::KEY_PG_DOWN, std::string("PG_DOWN") },
        { Keyboard::KEY_LEFT_ARROW, std::string("LEFT_ARROW") },
        { Keyboard::KEY_RIGHT_ARROW, std::string("RIGHT_ARROW") },
        { Keyboard::KEY_UP_ARROW, std::string("UP_ARROW") },
        { Keyboard::KEY_DOWN_ARROW, std::string("DOWN_ARROW") },
        { Keyboard::KEY_NUM_LOCK, std::string("NUM_LOCK") },
        { Keyboard::KEY_KP_PLUS, std::string("KP_PLUS") },
        { Keyboard::KEY_KP_MINUS, std::string("KP_MINUS") },
        { Keyboard::KEY_KP_MULTIPLY, std::string("KP_MULTIPLY") },
        { Keyboard::KEY_KP_DIVIDE, std::string("KP_DIVIDE") },
        { Keyboard::KEY_KP_ENTER, std::string("KP_ENTER") },
        { Keyboard::KEY_KP_HOME, std::string("KP_HOME") },
        { Keyboard::KEY_KP_UP, std::string("KP_UP") },
        { Keyboard::KEY_KP_PG_UP, std::string("KP_PG_UP") },
        { Keyboard::KEY_KP_LEFT, std::string("KP_LEFT") },
        { Keyboard::KEY_KP_FIVE, std::string("KP_FIVE") },
        { Keyboard::KEY_KP_RIGHT, std::string("KP_RIGHT") },
        { Keyboard::KEY_KP_END, std::string("KP_END") },
        { Keyboard::KEY_KP_DOWN, std::string("KP_DOWN") },
        { Keyboard::KEY_KP_PG_DOWN, std::string("KP_PG_DOWN") },
        { Keyboard::KEY_KP_INSERT, std::string("KP_INSERT") },
        { Keyboard::KEY_KP_DELETE, std::string("KP_DELETE") },
        { Keyboard::KEY_F1, std::string("F1") },
        { Keyboard::KEY_F2, std::string("F2") },
        { Keyboard::KEY_F3, std::string("F3") },
        { Keyboard::KEY_F4, std::string("F4") },
        { Keyboard::KEY_F5, std::string("F5") },
        { Keyboard::KEY_F6, std::string("F6") },
        { Keyboard::KEY_F7, std::string("F7") },
        { Keyboard::KEY_F8, std::string("F8") },
        { Keyboard::KEY_F9, std::string("F9") },
        { Keyboard::KEY_F10, std::string("F10") },
        { Keyboard::KEY_F11, std::string("F11") },
        { Keyboard::KEY_F12, std::string("F12") },
        { Keyboard::KEY_SPACE, std::string("SPACE") },
        { Keyboard::KEY_EXCLAM, std::string("!") },
        { Keyboard::KEY_QUOTE, std::string("\"") },
        { Keyboard::KEY_NUMBER, std::string("#") },
        { Keyboard::KEY_DOLLAR, std::string("$") },
        { Keyboard::KEY_PERCENT, std::string("%") },
        { Keyboard::KEY_CIRCUMFLEX, std::string("^") },
        { Keyboard::KEY_AMPERSAND, std::string("&") },
        { Keyboard::KEY_APOSTROPHE, std::string("'") },
        { Keyboard::KEY_LEFT_PARENTHESIS, std::string("(") },
        { Keyboard::KEY_RIGHT_PARENTHESIS, std::string(")") },
        { Keyboard::KEY_ASTERISK, std::string("*") },
        { Keyboard::KEY_PLUS, std::string("+") },
        { Keyboard::KEY_COMMA, std::string(",") },
        { Keyboard::KEY_MINUS, std::string("-") },
        { Keyboard::KEY_PERIOD, std::string(".") },
        { Keyboard::KEY_SLASH, std::string("/") },
        { Keyboard::KEY_ZERO, std::string("0") },
        { Keyboard::KEY_ONE, std::string("1") },
        { Keyboard::KEY_TWO, std::string("2") },
        { Keyboard::KEY_THREE, std::string("3") },
        { Keyboard::KEY_FOUR, std::string("4") },
        { Keyboard::KEY_FIVE, std::string("5") },
        { Keyboard::KEY_SIX, std::string("6") },
        { Keyboard::KEY_SEVEN, std::string("7") },
        { Keyboard::KEY_EIGHT, std::string("8") },
        { Keyboard::KEY_NINE, std::string("9") },
        { Keyboard::KEY_COLON, std::string(":") },
        { Keyboard::KEY_SEMICOLON, std::string(";") },
        { Keyboard::KEY_LESS_THAN, std::string("<") },
        { Keyboard::KEY_EQUAL, std::string("=") },
        { Keyboard::KEY_GREATER_THAN, std::string(">") },
        { Keyboard::KEY_QUESTION, std::string("?") },
        { Keyboard::KEY_AT, std::string("@") },
        { Keyboard::KEY_CAPITAL_A, std::string("A") },
        { Keyboard::KEY_CAPITAL_B, std::string("B") },
        { Keyboard::KEY_CAPITAL_C, std::string("C") },
        { Keyboard::KEY_CAPITAL_D, std::string("D") },
        { Keyboard::KEY_CAPITAL_E, std::string("E") },
        { Keyboard::KEY_CAPITAL_F, std::string("F") },
        { Keyboard::KEY_CAPITAL_G, std::string("G") },
        { Keyboard::KEY_CAPITAL_H, std::string("H") },
        { Keyboard::KEY_CAPITAL_I, std::string("I") },
        { Keyboard::KEY_CAPITAL_J, std::string("J") },
        { Keyboard::KEY_CAPITAL_K, std::string("K") },
        { Keyboard::KEY_CAPITAL_L, std::string("L") },
        { Keyboard::KEY_CAPITAL_M, std::string("M") },
        { Keyboard::KEY_CAPITAL_N, std::string("N") },
        { Keyboard::KEY_CAPITAL_O, std::string("O") },
        { Keyboard::KEY_CAPITAL_P, std::string("P") },
        { Keyboard::KEY_CAPITAL_Q, std::string("Q") },
        { Keyboard::KEY_CAPITAL_R, std::string("R") },
        { Keyboard::KEY_CAPITAL_S, std::string("S") },
        { Keyboard::KEY_CAPITAL_T, std::string("T") },
        { Keyboard::KEY_CAPITAL_U, std::string("U") },
        { Keyboard::KEY_CAPITAL_V, std::string("V") },
        { Keyboard::KEY_CAPITAL_W, std::string("W") },
        { Keyboard::KEY_CAPITAL_X, std::string("X") },
        { Keyboard::KEY_CAPITAL_Y, std::string("Y") },
        { Keyboard::KEY_CAPITAL_Z, std::string("Z") },
        { Keyboard::KEY_LEFT_BRACKET, std::string("[") },
        { Keyboard::KEY_BACK_SLASH, std::string("\\") },
        { Keyboard::KEY_RIGHT_BRACKET, std::string("]") },
        { Keyboard::KEY_UNDERSCORE, std::string("_") },
        { Keyboard::KEY_GRAVE, std::string("`") },
        { Keyboard::KEY_A, std::string("a") },
        { Keyboard::KEY_B, std::string("b") },
        { Keyboard::KEY_C, std::string("c") },
        { Keyboard::KEY_D, std::string("d") },
        { Keyboard::KEY_E, std::string("e") },
        { Keyboard::KEY_F, std::string("f") },
        { Keyboard::KEY_G, std::string("g") },
        { Keyboard::KEY_H, std::string("h") },
        { Keyboard::KEY_I, std::string("i") },
        { Keyboard::KEY_J, std::string("j") },
        { Keyboard::KEY_K, std::string("k") },
        { Keyboard::KEY_L, std::string("l") },
        { Keyboard::KEY_M, std::string("m") },
        { Keyboard::KEY_N, std::string("n") },
        { Keyboard::KEY_O, std::string("o") },
        { Keyboard::KEY_P, std::string("p") },
        { Keyboard::KEY_Q, std::string("q") },
        { Keyboard::KEY_R, std::string("r") },
        { Keyboard::KEY_S, std::string("s") },
        { Keyboard::KEY_T, std::string("t") },
        { Keyboard::KEY_U, std::string("u") },
        { Keyboard::KEY_V, std::string("v") },
        { Keyboard::KEY_W, std::string("w") },
        { Keyboard::KEY_X, std::string("x") },
        { Keyboard::KEY_Y, std::string("y") },
        { Keyboard::KEY_Z, std::string("z") },
        { Keyboard::KEY_LEFT_BRACE, std::string("{") },
        { Keyboard::KEY_BAR, std::string("|") },
        { Keyboard::KEY_RIGHT_BRACE, std::string("}") },
        { Keyboard::KEY_TILDE, std::string("~") },
        { Keyboard::KEY_EURO, std::string("EURO") },
        { Keyboard::KEY_POUND, std::string("POUND") },
        { Keyboard::KEY_YEN, std::string("YEN") },
        { Keyboard::KEY_MIDDLE_DOT, std::string("MIDDLE DOT") },
        { Keyboard::KEY_SEARCH, std::string("SEARCH") }
    };

    auto it = keyMap.find(key);
    return (it != keyMap.end()) ? it->second : EMPTY_STRING;
}
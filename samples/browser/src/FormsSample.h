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

using namespace tractor;

class FormsSample : public Sample, Control::Listener
{
  public:
    FormsSample();

    void gamepadEvent(Gamepad::GamepadEvent evt, Gamepad* gamepad);

  protected:
    void initialize();

    void finalize();

    void update(float elapsedTime);

    void render(float elapsedTime);

    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

    void controlEvent(Control* control, EventType evt);

  private:
    void formChanged();

    void createSampleForm();

    Scene* _scene{ nullptr };
    Node* _formNode{ nullptr };
    Node* _formNodeParent{ nullptr };
    Form* _formSelect{ nullptr };
    Form* _activeForm{ nullptr };
    std::vector<Form*> _forms;
    Gamepad* _gamepad{ nullptr };
    unsigned int _keyFlags{ 0 };
    bool _touched{ false };
    int _touchX{ 0 };
    unsigned int _formIndex{ 0 };
    std::vector<std::string> _formFiles{};
    std::string _sampleString{};
    Vector2 _joysticks[2];
};

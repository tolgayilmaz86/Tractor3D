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

#include "FirstPersonCamera.h"
#include "Sample.h"
#include "tractor.h"

using namespace tractor;

/**
 * Sample 3D audio within multiple sources and a listener bound to Nodes.
 */
class Audio3DSample : public Sample
{
  public:
    Audio3DSample();

    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

    void keyEvent(Keyboard::KeyEvent evt, int key);

    bool mouseEvent(Mouse::MouseEvent evt, int x, int y, int wheelDelta);

    void gamepadEvent(Gamepad::GamepadEvent evt, Gamepad* gamepad);

  protected:
    void initialize();

    void finalize();

    void update(float elapsedTime);

    void render(float elapsedTime);

  private:
    bool drawScene(Node* node);

    void addSound(const std::string& file);

    void drawDebugText(int x, int y, unsigned int fontSize);

    void drawVector3(const char* str, const Vector3& vector, int x, int y);

    void loadGrid(Scene* scene);

  private:
    FirstPersonCamera _fpCamera;
    std::map<std::string, Node*> _audioNodes;
    Font* _font;
    Scene* _scene;
    Node* _cubeNode;
    Gamepad* _gamepad;
    unsigned int _moveFlags;
    int _prevX;
    int _prevY;
    bool _buttonPressed;
};

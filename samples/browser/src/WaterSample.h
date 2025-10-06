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

#include "Sample.h"
#include "tractor.h"

using namespace tractor;

/**
 * Sample creating a water effect.
 */
class WaterSample : public Sample
{
  public:
    WaterSample();

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
    enum CameraMovement
    {
        MOVE_FORWARD = (1 << 0),
        MOVE_BACKWARD = (1 << 1),
        MOVE_LEFT = (1 << 2),
        MOVE_RIGHT = (1 << 3)
    };

    Font* _font;
    Scene* _scene;
    Node* _cameraNode;
    Node* _reflectCameraNode;

    Vector3 _cameraAcceleration;
    float _waterHeight;

    unsigned _inputMask;
    int _prevX, _prevY;

    FrameBuffer* _refractBuffer;
    SpriteBatch* _refractBatch;
    FrameBuffer* _reflectBuffer;
    SpriteBatch* _reflectBatch;

    bool _showBuffers;
    Vector4 _clipPlane;
    const Vector4& getClipPlane() const noexcept { return _clipPlane; }
    Matrix m_worldViewProjectionReflection;
    const Matrix& getReflectionMatrix() const noexcept { return m_worldViewProjectionReflection; }

    float getTime() const noexcept { return Game::getGameTime() * 0.0001; }

    Gamepad* _gamepad;
    bool drawScene(Node* node, bool drawWater);
};

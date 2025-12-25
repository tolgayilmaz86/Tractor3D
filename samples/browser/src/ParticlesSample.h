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
 * Main game class.
 */
class ParticlesSample : public Sample, Control::Listener
{
  public:
    /**
     * Constructor.
     */
    ParticlesSample() = default;

    /**
     * @see Game::touchEvent
     */
    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

    /**
     * @see Game::mouseEvent
     */
    bool mouseEvent(Mouse::MouseEvent evt, int x, int y, int wheelDelta);

    /**
     * @see Game::keyEvent
     */
    void keyEvent(Keyboard::KeyEvent evt, int key);

    /**
     * @see Game::resizeEvent
     */
    void resizeEvent(unsigned int width, unsigned int height);

    /**
     * @see Control::controlEvent
     */
    void controlEvent(Control* control, EventType evt);

  protected:
    /**
     * @see Game::initialize
     */
    void initialize();

    /**
     * @see Game::finalize
     */
    void finalize();

    /**
     * @see Game::update
     */
    void update(float elapsedTime);

    /**
     * @see Game::render
     */
    void render(float elapsedTime);

  private:
    bool drawScene(Node* node, void* cookie);

    void loadEmitters();

    void emitterChanged();

    void drawFrameRate(Font* font,
                       const Vector4& color,
                       unsigned int x,
                       unsigned int y,
                       unsigned int fps);

    void updateTexture();

    void updateImageControl();

    void updateFrames();

    void addGrid(unsigned int lineCount);

    void saveFile();

    std::string toString(bool b);

    std::string toString(int i);

    std::string toString(unsigned int i);

    std::string toString(const Vector3& v);

    std::string toString(const Vector4& v);

    std::string toString(const Quaternion& q);

    std::string toString(ParticleEmitter::BlendMode blendMode);

    ScenePtr _scene{ nullptr };
    Node* _cameraParent{ nullptr };
    Node* _particleEmitterNode{ nullptr };
    ParticleEmitterPtr _particleEmitter;
    bool _wDown{ false }, _sDown{ false }, _aDown{ false }, _dDown{ false };
    bool _touched{ false };
    int _prevX{ 0 }, _prevY{ 0 };
    bool _panning{ false };
    bool _rotating{ false };
    bool _zooming{ false };
    Font* _font{ nullptr };
    Form* _form{ nullptr };
    Slider* _startRed{ nullptr };
    Slider* _startGreen{ nullptr };
    Slider* _startBlue{ nullptr };
    Slider* _startAlpha{ nullptr };
    Slider* _endRed{ nullptr };
    Slider* _endGreen{ nullptr };
    Slider* _endBlue{ nullptr };
    Slider* _endAlpha{ nullptr };
    Slider* _startMin{ nullptr };
    Slider* _startMax{ nullptr };
    Slider* _endMin{ nullptr };
    Slider* _endMax{ nullptr };
    Slider* _energyMin{ nullptr };
    Slider* _energyMax{ nullptr };
    Slider* _emissionRate{ nullptr };
    Slider* _posVarX{ nullptr };
    Slider* _posVarY{ nullptr };
    Slider* _posVarZ{ nullptr };
    Slider* _velX{ nullptr };
    Slider* _velY{ nullptr };
    Slider* _velZ{ nullptr };
    Slider* _velVarX{ nullptr };
    Slider* _velVarY{ nullptr };
    Slider* _velVarZ{ nullptr };
    Slider* _accelX{ nullptr };
    Slider* _accelY{ nullptr };
    Slider* _accelZ{ nullptr };
    Slider* _accelVarX{ nullptr };
    Slider* _accelVarY{ nullptr };
    Slider* _accelVarZ{ nullptr };
    Slider* _spinSpeedMin{ nullptr };
    Slider* _spinSpeedMax{ nullptr };
    Slider* _axisX{ nullptr };
    Slider* _axisY{ nullptr };
    Slider* _axisZ{ nullptr };
    Slider* _axisVarX{ nullptr };
    Slider* _axisVarY{ nullptr };
    Slider* _axisVarZ{ nullptr };
    Slider* _rotationSpeedMin{ nullptr };
    Slider* _rotationSpeedMax{ nullptr };
    CheckBox* _started{ nullptr };
    Button* _reset{ nullptr };
    Button* _emit{ nullptr };
    Button* _zoomIn{ nullptr };
    Button* _zoomOut{ nullptr };
    Button* _save{ nullptr };
    Button* _load{ nullptr };
    Slider* _burstSize{ nullptr };
    Container* _position{ nullptr };
    Container* _particleProperties{ nullptr };
    CheckBox* _vsync{ nullptr };
    std::string _url{};
};

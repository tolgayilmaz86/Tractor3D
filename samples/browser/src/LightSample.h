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

#include "graphics/Model.h"
#include "renderer/Material.h"
#include "Sample.h"
#include "tractor.h"

using namespace tractor;

/**
 * Sample for lights.
 */
class LightSample : public Sample, Control::Listener
{
  public:
    LightSample() = default;

    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

    bool mouseEvent(Mouse::MouseEvent evt, int x, int y, int wheelDelta);

    void keyEvent(Keyboard::KeyEvent evt, int key);

    void controlEvent(Control* control, EventType evt);

  protected:
    void initialize();

    void finalize();

    void update(float elapsedTime);

    void render(float elapsedTime);

  private:
    void initializeDirectionalTechnique(const std::string& technique);

    void initializeSpotTechnique(const std::string& technique);

    void initializePointTechnique(const std::string& technique);

    void setUnlitMaterialTexture(Model* model, const std::string& texturePath, bool mipmap = true);

    void setColorValue(const Vector3& value);

    void setSpecularValue(float);

    bool drawScene(Node* node);

    FontPtr _font;
    ScenePtr _scene;
    Node* _modelNode{nullptr};
    Node* _directionalLightNode{nullptr};
    Node* _pointLightNode{nullptr};
    Node* _spotLightNode{nullptr};
    Node* _usedForMoving{nullptr};

    Model* _model{nullptr};
    ModelPtr _directionalLightQuadModel;
    ModelPtr _spotLightQuadModel;
    ModelPtr _pointLightQuadModel{nullptr};

    Material* _unlitMaterial{nullptr};
    Material* _texturedMaterial{nullptr};
    Material* _bumpedMaterial{nullptr};
    Material* _bumpedSpecularMaterial{nullptr};

    MaterialPtr _lighting;

    RadioButton* _noLight{nullptr};
    RadioButton* _directional{nullptr};
    RadioButton* _spot{nullptr};
    RadioButton* _point{nullptr};

    Container* _properties{nullptr};
    Slider* _redSlider{nullptr};
    Slider* _greenSlider{nullptr};
    Slider* _blueSlider{nullptr};
    Slider* _specularSlider{nullptr};
    CheckBox* _addSpecular{nullptr};
    CheckBox* _addBumped{nullptr};

    FormPtr _form;

    bool _touched{false};
    int _touchX{0}, _touchY{0};
};

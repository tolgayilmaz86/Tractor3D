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
    LightSample();

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

    Font* _font;
    Scene* _scene;
    Node* _modelNode;
    Node* _directionalLightNode;
    Node* _pointLightNode;
    Node* _spotLightNode;
    Node* _usedForMoving;

    Model* _model;
    Model* _directionalLightQuadModel;
    Model* _spotLightQuadModel;
    Model* _pointLightQuadModel;

    Material* _unlitMaterial;
    Material* _texturedMaterial;
    Material* _bumpedMaterial;
    Material* _bumpedSpecularMaterial;

    Material* _lighting;

    RadioButton* _noLight;
    RadioButton* _directional;
    RadioButton* _spot;
    RadioButton* _point;

    Container* _properties;
    Slider* _redSlider;
    Slider* _greenSlider;
    Slider* _blueSlider;
    Slider* _specularSlider;
    CheckBox* _addSpecular;
    CheckBox* _addBumped;

    Form* _form;

    bool _touched;
    int _touchX, _touchY;
};

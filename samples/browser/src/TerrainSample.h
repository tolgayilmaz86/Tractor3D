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

class TerrainSample : public Sample, public Control::Listener, private RenderState::AutoBindingResolver
{
  public:
    TerrainSample();

    ~TerrainSample();

    void keyEvent(Keyboard::KeyEvent evt, int key);

    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

    bool mouseEvent(Mouse::MouseEvent evt, int x, int y, int wheelDelta);

    void controlEvent(Control* control, EventType evt);

  protected:
    void initialize();

    bool intializeLights(Node* node);

    void finalize();

    void update(float elapsedTime);

    void render(float elapsedTime);

    bool drawScene(Node* node);

    void setMessage(const std::string& message);

  private:
    enum Mode
    {
        MODE_LOOK,
        MODE_DROP_SPHERE,
        MODE_DROP_BOX
    };

    Vector3 getLightDirection0() const;
    Vector3 getLightColor0() const noexcept;

    bool resolveAutoBinding(const std::string& autoBinding, Node* node, MaterialParameter* parameter);

    Font* _font;
    Scene* _scene;
    Terrain* _terrain;
    Node* _sky;
    Form* _form;
    bool _formVisible;
    Vector2 _formSize;
    bool _wireframe;
    bool _debugPhysics;
    bool _snapToGround;
    bool _vsync;
    Mode _mode;
    Node* _sphere;
    Node* _box;
    std::list<Node*> _shapes;
    Light* _directionalLight;
};

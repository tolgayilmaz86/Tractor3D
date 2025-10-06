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
 * Sample loading a physics scene from .scene file with .physics bindings
 */
class PhysicsCollisionObjectSample : public Sample, Control::Listener
{
  public:
    PhysicsCollisionObjectSample();

    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

    void keyEvent(Keyboard::KeyEvent evt, int key);

    void controlEvent(Control* control, EventType evt);

  protected:
    void initialize();

    void finalize();

    void update(float elapsedTime);

    void render(float elapsedTime);

  private:
    bool drawScene(Node* node);

    bool bindLights(Node* node);

    void fireProjectile(const Ray& ray);

    void incrementDebugDraw();

    void toggleWireframe();

    enum ObjectsTypes
    {
        SPHERE = 0,
        BOX = 1,
        CAPSULE = 2,
        DUCK = 3
    };

    Font* _font;
    Scene* _scene;
    Node* _lightNode;
    Form* _form;
    int _objectType;
    bool _throw;
    int _drawDebug;
    bool _wireFrame;
    std::vector<std::string> _collisionObjectPaths;
    std::vector<std::string> _nodeIds;
    std::vector<std::string> _nodeNames;
    std::vector<Vector4> _colors;
};

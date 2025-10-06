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
#include "FirstPersonCamera.h"

FirstPersonCamera::FirstPersonCamera() : _pitchNode(nullptr), _rootNode(nullptr) {}

FirstPersonCamera::~FirstPersonCamera()
{
    SAFE_RELEASE(_pitchNode);
    SAFE_RELEASE(_rootNode);
}

void FirstPersonCamera::initialize(float nearPlane, float farPlane, float fov)
{
    SAFE_RELEASE(_pitchNode);
    SAFE_RELEASE(_rootNode);
    _rootNode = Node::create("FirstPersonCamera_root");
    _pitchNode = Node::create("FirstPersonCamera_pitch");
    _rootNode->addChild(_pitchNode);

    float aspectRatio = Game::getInstance()->getAspectRatio();
    assert(aspectRatio > 0.0f);
    Camera* camera = Camera::createPerspective(fov, aspectRatio, nearPlane, farPlane);
    _pitchNode->setCamera(camera);
    SAFE_RELEASE(camera);
}

Node* FirstPersonCamera::getRootNode() { return _rootNode; }

Camera* FirstPersonCamera::getCamera()
{
    if (_pitchNode) return _pitchNode->getCamera();
    return nullptr;
}

void FirstPersonCamera::setPosition(const Vector3& position)
{
    _rootNode->setTranslation(position);
}

void FirstPersonCamera::moveForward(float amount)
{
    Vector3 v = _pitchNode->getForwardVectorWorld();
    v.normalize().scale(amount);
    _rootNode->translate(v);
}

void FirstPersonCamera::moveBackward(float amount) { moveForward(-amount); }

void FirstPersonCamera::moveLeft(float amount) { _rootNode->translateLeft(amount); }

void FirstPersonCamera::moveRight(float amount) { _rootNode->translateLeft(-amount); }

void FirstPersonCamera::moveUp(float amount) { _rootNode->translateUp(amount); }

void FirstPersonCamera::moveDown(float amount) { _rootNode->translateUp(-amount); }

void FirstPersonCamera::rotate(float yaw, float pitch)
{
    _rootNode->rotateY(-yaw);
    _pitchNode->rotateX(pitch);
}

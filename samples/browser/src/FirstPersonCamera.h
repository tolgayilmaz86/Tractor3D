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

#include "tractor.h"

using namespace tractor;

/**
 * FirstPersonCamera controls a camera like a first person shooter game.
 */
class FirstPersonCamera
{
  public:
    /**
     * Constructor.
     */
    FirstPersonCamera();

    /**
     * Destructor.
     */
    ~FirstPersonCamera();

    /**
     * Initializes the first person camera. Should be called after the Game has been initialized.
     */
    void initialize(float nearPlane = 1.0f, float farPlane = 1000.0f, float fov = 45.0f);

    /**
     * Gets root node. May be nullptr if not initialized.
     *
     * @return Root node or nullptr.
     */
    Node* getRootNode();

    /**
     * Gets the camera. May be nullptr.
     *
     * @return Camera or nullptr.
     */
    Camera* getCamera();

    /**
     * Sets the position of the camera.
     *
     * @param position The position to move to.
     */
    void setPosition(const Vector3& position);

    /**
     * Moves the camera forward in the direction that it is pointing. (Fly mode)
     */
    void moveForward(float amount);

    /**
     * Moves the camera in the opposite direction that it is pointing.
     */
    void moveBackward(float amount);

    /**
     * Strafes that camera left, which is perpendicular to the direction it is facing.
     */
    void moveLeft(float amount);

    /**
     * Strafes that camera right, which is perpendicular to the direction it is facing.
     */
    void moveRight(float amount);

    void moveUp(float amount);

    void moveDown(float amount);

    /**
     * Rotates the camera in place in order to change the direction it is looking.
     *
     * @param yaw Rotates the camera around the yaw axis in radians. Positive looks right, negative looks left.
     * @param pitch Rotates the camera around the ptich axis in radians. Positive looks up, negative looks down.
     */
    void rotate(float yaw, float pitch);

  private:
    Node* _pitchNode;
    Node* _rootNode;
};

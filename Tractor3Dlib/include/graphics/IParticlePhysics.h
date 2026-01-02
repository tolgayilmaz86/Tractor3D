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

#include "math/Vector3.h"
#include "math/Vector4.h"

namespace tractor
{

/**
 * Interface for particle physics/motion properties.
 * 
 * This interface follows the Interface Segregation Principle by separating
 * physics/motion configuration from emission control and rendering concerns.
 * 
 * Clients that need to configure particle physics (velocity, acceleration, rotation)
 * can depend on this interface without coupling to full ParticleEmitter functionality.
 */
class IParticlePhysics
{
public:
    virtual ~IParticlePhysics() = default;

    // Position properties

    /**
     * Sets the initial position and position variance of new particles.
     *
     * @param position The initial position of new particles.
     * @param positionVariance The amount of variance allowed in the initial position.
     */
    virtual void setPosition(const Vector3& position, const Vector3& positionVariance) = 0;

    /**
     * Gets the position of new particles, relative to the emitter's transform.
     *
     * @return The position of new particles.
     */
    virtual const Vector3& getPosition() const noexcept = 0;

    /**
     * Gets the position variance of new particles.
     *
     * @return The position variance of new particles.
     */
    virtual const Vector3& getPositionVariance() const noexcept = 0;

    /**
     * Sets whether positions are generated within an ellipsoidal domain.
     *
     * @param ellipsoid Whether to use ellipsoidal domain for initial positions.
     */
    virtual void setEllipsoid(bool ellipsoid) = 0;

    /**
     * Determines whether positions are generated within an ellipsoidal domain.
     *
     * @return true if ellipsoid mode is enabled, false otherwise.
     */
    virtual bool isEllipsoid() const noexcept = 0;

    // Velocity properties

    /**
     * Sets the base velocity of new particles and its variance.
     *
     * @param velocity The initial velocity of new particles.
     * @param velocityVariance The amount of variance allowed in the initial velocity.
     */
    virtual void setVelocity(const Vector3& velocity, const Vector3& velocityVariance) = 0;

    /**
     * Gets the initial velocity of new particles.
     *
     * @return The initial velocity of new particles.
     */
    virtual const Vector3& getVelocity() const noexcept = 0;

    /**
     * Gets the initial velocity variance of new particles.
     *
     * @return The initial velocity variance of new particles.
     */
    virtual const Vector3& getVelocityVariance() const noexcept = 0;

    // Acceleration properties

    /**
     * Sets the base acceleration vector and its allowed variance.
     *
     * @param acceleration The base acceleration vector of emitted particles.
     * @param accelerationVariance The variance allowed in the acceleration.
     */
    virtual void setAcceleration(const Vector3& acceleration, const Vector3& accelerationVariance) = 0;

    /**
     * Gets the base acceleration vector of particles.
     *
     * @return The base acceleration vector of particles.
     */
    virtual const Vector3& getAcceleration() const noexcept = 0;

    /**
     * Gets the variance of acceleration of particles.
     *
     * @return The variance of acceleration of particles.
     */
    virtual const Vector3& getAccelerationVariance() const noexcept = 0;

    // Rotation properties

    /**
     * Sets the per-particle rotation speed range.
     *
     * @param speedMin The minimum rotation speed (per particle).
     * @param speedMax The maximum rotation speed (per particle).
     */
    virtual void setRotationPerParticle(float speedMin, float speedMax) = 0;

    /**
     * Gets the minimum rotation speed of each emitted particle.
     *
     * @return The minimum rotation speed of each emitted particle.
     */
    virtual float getRotationPerParticleSpeedMin() const = 0;

    /**
     * Gets the maximum rotation speed of each emitted particle.
     *
     * @return The maximum rotation speed of each emitted particle.
     */
    virtual float getRotationPerParticleSpeedMax() const = 0;

    /**
     * Sets the rotation axis in world space around which all particles will spin.
     *
     * @param speedMin The minimum rotation speed of emitted particles.
     * @param speedMax The maximum rotation speed of emitted particles.
     * @param axis The base rotation axis of emitted particles.
     * @param axisVariance The variance of the rotation axis.
     */
    virtual void setRotation(float speedMin, float speedMax, 
                             const Vector3& axis, const Vector3& axisVariance) = 0;

    /**
     * Gets the minimum rotation speed of emitted particles.
     *
     * @return The minimum rotation speed.
     */
    virtual float getRotationSpeedMin() const noexcept = 0;

    /**
     * Gets the maximum rotation speed of emitted particles.
     *
     * @return The maximum rotation speed.
     */
    virtual float getRotationSpeedMax() const noexcept = 0;

    /**
     * Gets the base rotation axis of emitted particles.
     *
     * @return The base rotation axis.
     */
    virtual const Vector3& getRotationAxis() const noexcept = 0;

    /**
     * Gets the variance of the rotation axis.
     *
     * @return The variance of the rotation axis.
     */
    virtual const Vector3& getRotationAxisVariance() const noexcept = 0;

    // Orbit properties

    /**
     * Sets whether vector properties are rotated by the node's rotation matrix.
     *
     * @param orbitPosition Whether to rotate initial positions.
     * @param orbitVelocity Whether to rotate initial velocities.
     * @param orbitAcceleration Whether to rotate initial accelerations.
     */
    virtual void setOrbit(bool orbitPosition, bool orbitVelocity, bool orbitAcceleration) = 0;

    /**
     * Whether new particle positions are rotated by the node's rotation matrix.
     *
     * @return True if orbiting positions, false otherwise.
     */
    virtual bool getOrbitPosition() const noexcept = 0;

    /**
     * Whether new particle velocities are rotated by the node's rotation matrix.
     *
     * @return True if orbiting velocities, false otherwise.
     */
    virtual bool getOrbitVelocity() const noexcept = 0;

    /**
     * Whether new particle accelerations are rotated by the node's rotation matrix.
     *
     * @return True if orbiting accelerations, false otherwise.
     */
    virtual bool getOrbitAcceleration() const noexcept = 0;
};

} // namespace tractor

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

#include "math/Vector4.h"

namespace tractor
{

/**
 * Interface for particle appearance properties (size, color, energy/lifetime).
 * 
 * This interface follows the Interface Segregation Principle by separating
 * visual appearance configuration from physics and emission concerns.
 * 
 * Clients that need to configure particle visuals (size, color, lifetime)
 * can depend on this interface without coupling to full ParticleEmitter functionality.
 */
class IParticleAppearance
{
public:
    virtual ~IParticleAppearance() = default;

    // Size properties

    /**
     * Sets the minimum and maximum size at spawn and end of lifetime.
     *
     * @param startMin The minimum size when spawned.
     * @param startMax The maximum size when spawned.
     * @param endMin The minimum size at end of lifetime.
     * @param endMax The maximum size at end of lifetime.
     */
    virtual void setSize(float startMin, float startMax, float endMin, float endMax) = 0;

    /**
     * Gets the minimum size when spawned.
     *
     * @return The minimum spawn size.
     */
    virtual float getSizeStartMin() const noexcept = 0;

    /**
     * Gets the maximum size when spawned.
     *
     * @return The maximum spawn size.
     */
    virtual float getSizeStartMax() const noexcept = 0;

    /**
     * Gets the minimum size at end of lifetime.
     *
     * @return The minimum end size.
     */
    virtual float getSizeEndMin() const noexcept = 0;

    /**
     * Gets the maximum size at end of lifetime.
     *
     * @return The maximum end size.
     */
    virtual float getSizeEndMax() const noexcept = 0;

    // Color properties

    /**
     * Sets the start and end colors, and their variances.
     *
     * @param start The base start color.
     * @param startVariance The variance of start color.
     * @param end The base end color.
     * @param endVariance The variance of end color.
     */
    virtual void setColor(const Vector4& start,
                          const Vector4& startVariance,
                          const Vector4& end,
                          const Vector4& endVariance) = 0;

    /**
     * Gets the base start color.
     *
     * @return The base start color.
     */
    virtual const Vector4& getColorStart() const noexcept = 0;

    /**
     * Gets the variance of start color.
     *
     * @return The variance of start color.
     */
    virtual const Vector4& getColorStartVariance() const noexcept = 0;

    /**
     * Gets the base end color.
     *
     * @return The base end color.
     */
    virtual const Vector4& getColorEnd() const noexcept = 0;

    /**
     * Gets the variance of end color.
     *
     * @return The variance of end color.
     */
    virtual const Vector4& getColorEndVariance() const noexcept = 0;

    // Energy/Lifetime properties

    /**
     * Sets the minimum and maximum lifetime, measured in milliseconds.
     *
     * @param energyMin The minimum lifetime.
     * @param energyMax The maximum lifetime.
     */
    virtual void setEnergy(long energyMin, long energyMax) = 0;

    /**
     * Gets the minimum lifetime in milliseconds.
     *
     * @return The minimum lifetime.
     */
    virtual long getEnergyMin() const noexcept = 0;

    /**
     * Gets the maximum lifetime in milliseconds.
     *
     * @return The maximum lifetime.
     */
    virtual long getEnergyMax() const noexcept = 0;
};

} // namespace tractor

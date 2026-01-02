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

namespace tractor
{

/**
 * Interface for particle emission control.
 * 
 * This interface follows the Interface Segregation Principle by separating
 * emission control concerns from other particle emitter functionality like
 * rendering, physics properties, or sprite animation.
 * 
 * Clients that only need to control emission (start/stop/emit) can depend
 * on this interface without coupling to the full ParticleEmitter class.
 */
class IParticleEmission
{
public:
    virtual ~IParticleEmission() = default;

    /**
     * Starts emitting particles over time at the configured emission rate.
     */
    virtual void start() = 0;

    /**
     * Stops emitting particles over time.
     */
    virtual void stop() = 0;

    /**
     * Gets whether the emitter is currently started.
     *
     * @return Whether the emitter is currently started.
     */
    virtual bool isStarted() const noexcept = 0;

    /**
     * Gets whether the emitter is currently active (has any live particles).
     *
     * @return Whether the emitter is currently active.
     */
    virtual bool isActive() const = 0;

    /**
     * Generates an arbitrary number of particles all at once.
     *
     * @param particleCount The number of particles to emit immediately.
     */
    virtual void emitOnce(unsigned int particleCount) = 0;

    /**
     * Gets the current number of particles.
     *
     * @return The number of particles that are currently alive.
     */
    virtual unsigned int getParticlesCount() const noexcept = 0;

    /**
     * Sets the emission rate, measured in particles per second.
     *
     * @param rate The emission rate, measured in particles per second.
     */
    virtual void setEmissionRate(unsigned int rate) = 0;

    /**
     * Gets the emission rate, measured in particles per second.
     *
     * @return The emission rate, measured in particles per second.
     */
    virtual unsigned int getEmissionRate() const noexcept = 0;

    /**
     * Sets the maximum number of particles that can be emitted.
     *
     * @param max The maximum number of particles that can be emitted.
     */
    virtual void setParticleCountMax(unsigned int max) = 0;

    /**
     * Returns the maximum number of particles that can be emitted.
     *
     * @return The maximum number of particles that can be emitted.
     */
    virtual unsigned int getParticleCountMax() const noexcept = 0;
};

} // namespace tractor

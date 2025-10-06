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

#include "graphics/Drawable.h"
#include "graphics/Mesh.h"
#include "graphics/Rectangle.h"
#include "graphics/SpriteBatch.h"
#include "math/Transform.h"
#include "math/Vector2.h"
#include "math/Vector4.h"
#include "renderer/Texture.h"
#include "scene/Properties.h"

namespace tractor
{

class Node;

/**
 * Defines a particle emitter that can be made to simulate and render a particle system.
 *
 * Once created, the emitter can be set on a node in order to follow an object or be placed
 * within a scene.
 *
 * A ParticleEmitter has a texture and a maximum number of particles that can be alive at
 * once, both of which are set when the ParticleEmitter is created and cannot be changed
 * from then on.  Particles are rendered as camera-facing billboards using the emitter's
 * texture.  The ParticleEmitter's texture properties determine whether the texture
 * is treated as a single image, a texture atlas or an animated sprite.
 *
 * A ParticleEmitter also has a number of properties that determine values assigned to
 * individual particles it emits.  Scalar properties such as particle begin- and end-size
 * are assigned within a minimum and maximum value; vector properties are assigned within
 * the domain defined by a base vector and a variance vector as follows: The variance vector
 * is multiplied by a random scalar between 1 and -1, and the base vector is added to this
 * result.  This allows a ParticleEmitter to be created which emits particles with properties
 * that are randomized, yet fit within a well-defined range.  To make a property deterministic,
 * simply set the minimum to the same value as the maximum for that property or set its
 * variance to the zero vector.
 *
 * <h2>Scalar properties:</h2>
 *
 * Begin-Size: \n
 * The size of a newly emitted particle.
 *
 * End-Size: \n
 * The size of a particle at the end of its lifetime.  A particle's size will
 * interpolate linearly between its begin-size and end-size over its lifetime.
 *
 * Energy: \n
 * The length of time a particle will remain alive for.
 *
 * RotationSpeedPerParticle: \n
 * The speed and direction a particle will spin.  Since particles are
 * rendered as billboards, no axis of rotation can be specified per particle.
 * Each particles rotates around their center points, around the z-axis in
 * screen space.
 *
 * RotationSpeed:\n
 * The speed a particle will spin around its RotationAxis in world space.
 * (See RotationAxis under "Vector properties" below.)
 *
 *
 * <h2>Vector properties:</h2>
 *
 * Initial Position: \n
 * The position of a new particle at the moment it is emitted, relative
 * to the node its ParticleEmitter is set on.  This property is unique
 * in that the initial positions of new particles can be restricted to
 * fit within an ellipsoidal domain; see setEllipsoid().
 *
 * Initial Velocity: \n
 * The velocity of a new particle at the moment it is emitted.  This
 * property is measured in world coordinates per second and modifies
 * a particle's current position each time ParticleEmitter::update()
 * is called.
 *
 * Acceleration:\n
 * The particle's change in velocity, measured in world coordinates per second.
 * This property modifies a particle's current position each time
 * ParticleEmitter::update() is called.
 *
 * Color: \n
 * The color of a particle at the end of its lifetime.  A particle's color
 * will interpolate linearly between its begin-color and end-color over its lifetime.
 *
 * RotationAxis: \n
 * An axis in world space around which all particles will spin, allowing for tornado and
 * spiral effects.
 *
 * The vector properties Initial Position, Initial Velocity and Acceleration can be set to
 * orbit around the origin of a node a ParticleEmitter is set on by that node's rotation matrix.
 * This allows the rotation of a node, and not just its position, to affect these properties of
 * newly emitted particles.  An example of where this would be useful would be a water-fountain
 * emitter attached to the nozzle of a hose.  The initial position and initial velocity would be
 * set to orbit around the node's origin so that the water would always spray out in the direction
 * the nozzle was facing.  However, acceleration would not be set to orbit the node's origin in
 * order for gravity to continue to act in the same direction on water particles, no matter
 * what direction they were originally aimed.
 *
 * <h2>Rendering properties:</h2>
 *
 * Particles are rendered as screen-facing billboards -- that is, the ParticleEmitter's texture
 * is used to render particles as images that always face the camera.  For the simplest case,
 * where the entire texture is used for every particle, the default texture settings can be used.
 * However, a ParticleEmitter can also be configured to select one of several frames at random
 * for each particle, or to render each particle as a sprite that animates through the frames
 * over the course of its lifetime.
 *
 * Frame Count: \n
 * The number of individual images / frames contained in the texture.
 *
 * Texture Coordinates: \n
 * The coordinates within the texture used to render a specific frame.
 * Using a texture that places the frames together, without padding,
 * in left-to-right top-to-bottom order is recommended, as there is a utility
 * method for generating the texture coordinates for such a texture atlas /
 * sprite-map.  See setSpriteFrameCoords().
 *
 * Sprite Animating: \n
 * Set this to enable sprite animation.
 *
 * Sprite Looped: \n
 * If sprites are set to loop, each frame will last for the emitter's frameDuration.
 * If sprites are set not to loop, the animation will be timed so that the last frame
 * finishes just as a particle dies.  This setting has no effect if the sprite is not
 * animating.
 *
 * Sprite Random Offset: \n
 * New particles are created with one of the sprite frames in the emitter's texture.
 * If a maximum offset is set, a random frame from 0 to maxOffset will be selected.
 * If sprite animation is disabled and this offset is set to Frame Count, each
 * particle will use one of the sprite frames for its entire lifetime.
 *
 * Blend Mode: \n
 * Sets the blend mode used by this particle emitter.  The given blend factors will
 * be set before rendering the particle system and then will be reset to their original
 * values.  Accepts the same symbolic constants as glBlendFunc().
 */
class ParticleEmitter : public Ref, public Drawable
{
    friend class Node;

  public:
    /**
     * Defines the types of blend modes
     */
    enum BlendMode
    {
        BLEND_NONE,
        BLEND_ALPHA,
        BLEND_ADDITIVE,
        BLEND_MULTIPLIED
    };

    /**
     * Creates a particle emitter using the data from the Properties object defined at the specified
     * URL, where the URL is of the format
     * "<file-path>.<extension>#<namespace-id>/<namespace-id>/.../<namespace-id>" (and
     * "#<namespace-id>/<namespace-id>/.../<namespace-id>" is optional).
     *
     * @param url The URL pointing to the Properties object defining the particle emitter.
     *
     * @return An initialized ParticleEmitter.
     * @script{create}
     */
    static ParticleEmitter* create(const std::string& url);

    /**
     * Creates a particle emitter from the specified properties object.
     *
     * @param properties The properties object defining the
     *      particle emitter (must have namespace equal to 'particle').
     * @return The newly created particle emitter, or <code>nullptr</code> if the particle emitter failed to load.
     * @script{create}
     */
    static ParticleEmitter* create(Properties* properties);

    /**
     * Creates an uninitialized ParticleEmitter.
     *
     * @param texturePath A path to the image to use as this ParticleEmitter's texture.
     * @param blendMode The blend mode to be used for the particles emitted.
     * @param particleCountMax The maximum number of particles that can be alive at one time in this
     * ParticleEmitter's system.
     * @script{create}
     */
    static ParticleEmitter* create(const std::string& texturePath,
                                   BlendMode blendMode,
                                   unsigned int particleCountMax);

    /**
     * Sets a new texture for this particle emitter.
     *
     * The current texture's reference count is decreased.
     *
     * @param texturePath Path to the new texture to set.
     * @param blendMode Blend mode for the new texture.
     */
    void setTexture(const std::string& texturePath, BlendMode blendMode);

    /**
     * Sets a new texture for this particle emitter.
     *
     * The reference count of the specified texture is increased, and the
     * current texture's reference count is decreased.
     *
     * @param texture The new texture to set.
     * @param blendMode Blend mode for the new texture.
     */
    void setTexture(Texture* texture, BlendMode blendMode);

    /**
     * Returns the texture currently set for this particle emitter.
     *
     * @return The current texture.
     */
    Texture* getTexture() const;

    /**
     * Sets the maximum number of particles that can be emitted.
     *
     * @param max The maximum number of particles that can be emitted.
     */
    void setParticleCountMax(unsigned int max) { _particleCountMax = max; }

    /**
     * Returns the maximum number of particles that can be emitted.
     *
     * @return The maximum number of particles that can be emitted.
     */
    unsigned int getParticleCountMax() const noexcept { return _particleCountMax; }

    /**
     * Sets the emission rate, measured in particles per second.
     *
     * @param rate The emission rate, measured in particles per second.
     */
    void setEmissionRate(unsigned int rate);

    /**
     * Gets the emission rate, measured in particles per second.
     *
     * @return The emission rate, measured in particles per second.
     */
    unsigned int getEmissionRate() const noexcept { return _emissionRate; }

    /**
     * Starts emitting particles over time at this ParticleEmitter's emission rate.
     *
     * @see ParticleEmitter::emit()
     */
    void start();

    /**
     * Stops emitting particles over time.
     *
     * @see ParticleEmitter::emit()
     */
    void stop() { _started = false; }

    /**
     * Gets whether this ParticleEmitter is currently started.
     *
     * @return Whether this ParticleEmitter is currently started.
     */
    bool isStarted() const noexcept { return _started; }

    /**
     * Gets whether this ParticleEmitter is currently active (i.e. if any of its particles are alive).
     *
     * @return Whether this ParticleEmitter is currently active.
     */
    bool isActive() const;

    /**
     * Generates an arbitrary number of particles all at once.  Each newly emitted
     * particle has its properties assigned within the ranges defined by its ParticleEmitter.
     *
     * Note that the maximum number of particles that can be alive at once in a particle
     * system is defined when a ParticleEmitter is created and cannot be changed.  A call
     * to emit() cannot cause the particle system to exceed this maximum, so fewer or zero
     * particles will be emitted if the maximum is or has been reached.
     *
     * @param particleCount The number of particles to emit immediately.
     */
    void emitOnce(unsigned int particleCount);

    /**
     * Gets the current number of particles.
     *
     * @return The number of particles that are currently alive.
     */
    unsigned int getParticlesCount() const noexcept { return _particleCount; }

    /**
     * Sets whether the positions of newly emitted particles are generated within an ellipsoidal
     * domain.
     *
     * Each vector property is generated such as to fall within the domain defined by a base vector
     * and a variance vector. If that domain is ellipsoidal, vectors are generated within an
     * ellipsoid centered at the base vector and scaled by the variance vector. If that domain is
     * not ellipsoidal, vectors are generated by multiplying the variance vector by a random
     * floating-point number between -1 and 1, then adding this result to the base vector.
     *
     * Ellipsoidal domains are somewhat less efficient and only necessary when determining the
     * positions of newly emitted particles.  Call this method with 'true' to make initial position
     * an ellipsoidal domain. The default setting is 'false'.
     *
     * @param ellipsoid Whether initial particle positions are generated within an ellipsoidal
     * domain.
     */
    void setEllipsoid(bool ellipsoid) { _ellipsoid = ellipsoid; }

    /**
     * Determines whether the positions of newly emitted particles are generated within an ellipsoidal domain.
     *
     * @return true if is ellipsoid, false if not.
     */
    bool isEllipsoid() const noexcept { return _ellipsoid; }

    /**
     * Sets the minimum and maximum size that each particle can be at the time when it is spawned,
     * as well as the minimum and maximum size for particles to be at the end of their lifetimes.
     *
     * @param startMin The minimum size that each particle can be at the time when it is started.
     * @param startMax The maximum size that each particle can be at the time when it is started.
     * @param endMin The minimum size that each particle can be at the end of its lifetime.
     * @param endMax The maximum size that each particle can be at the end of its lifetime.
     */
    void setSize(float startMin, float startMax, float endMin, float endMax);

    /**
     * Gets the minimum size that each particle can be at the time when it is started.
     *
     * @return The minimum size that each particle can be at the time when it is started.
     */
    float getSizeStartMin() const noexcept { return _sizeStartMin; }

    /**
     * Gets the maximum size that each particle can be at the time when it is started.
     *
     * @return The maximum size that each particle can be at the time when it is started.
     */
    float getSizeStartMax() const noexcept { return _sizeStartMax; }

    /**
     * Gets the minimum size that each particle can be at the end of its lifetime.
     *
     * @return The minimum size that each particle can be at the end of its lifetime.
     */
    float getSizeEndMin() const noexcept { return _sizeEndMin; }

    /**
     * Gets the maximum size that each particle can be at the end of its lifetime.
     *
     * @return The maximum size that each particle can be at the end of its lifetime.
     */
    float getSizeEndMax() const noexcept { return _sizeEndMax; }

    /**
     * Set the start and end colors, and their variances, of particles in this emitter's system.
     *
     * @param start The base start color of emitted particles.
     * @param startVariance The variance of start color of emitted particles.
     * @param end The base end color of emitted particles.
     * @param endVariance The variance of end color of emitted particles.
     */
    void setColor(const Vector4& start,
                  const Vector4& startVariance,
                  const Vector4& end,
                  const Vector4& endVariance);

    /**
     * Gets the base start color of emitted particles.
     *
     * @return The base start color of emitted particles.
     */
    const Vector4& getColorStart() const noexcept { return _colorStart; }

    /**
     * Gets the variance of start color of emitted particles.
     *
     * @return The variance of start color of emitted particles.
     */
    const Vector4& getColorStartVariance() const noexcept { return _colorStartVar; }

    /**
     * Gets the base end color of emitted particles.
     *
     * @return The base end color of emitted particles.
     */
    const Vector4& getColorEnd() const noexcept { return _colorEnd; }

    /**
     * Gets the variance of end color of emitted particles.
     *
     * @return The variance of end color of emitted particles.
     */
    const Vector4& getColorEndVariance() const noexcept { return _colorEndVar; }

    /**
     * Sets the minimum and maximum lifetime of emitted particles, measured in milliseconds.
     *
     * @param energyMin The minimum lifetime of each particle, measured in milliseconds.
     * @param energyMax The maximum lifetime of each particle, measured in milliseconds.
     */
    void setEnergy(long energyMin, long energyMax);

    /**
     * Gets the minimum lifetime of each particle, measured in milliseconds.
     *
     * @return The minimum lifetime of each particle, measured in milliseconds.
     */
    long getEnergyMin() const noexcept { return _energyMin; }

    /**
     * Gets the maximum lifetime of each particle, measured in milliseconds.
     *
     * @return The maximum lifetime of each particle, measured in milliseconds.
     */
    long getEnergyMax() const noexcept { return _energyMax; }

    /**
     * Sets the initial position and position variance of new particles.
     *
     * @param position The initial position of new particles.
     * @param positionVariance The amount of variance allowed in the initial position of new particles.
     */
    void setPosition(const Vector3& position, const Vector3& positionVariance);

    /**
     * Gets the position of new particles, relative to the emitter's transform.
     *
     * @return The position of new particles, relative to the emitter's transform.
     */
    const Vector3& getPosition() const noexcept { return _position; }

    /**
     * Gets the position variance of new particles.
     *
     * @return The position variance of new particles.
     */
    const Vector3& getPositionVariance() const noexcept { return _positionVar; }

    /**
     * Sets the base velocity of new particles and its variance.
     *
     * @param velocity The initial velocity of new particles.
     * @param velocityVariance The amount of variance allowed in the initial velocity of new particles.
     */
    void setVelocity(const Vector3& velocity, const Vector3& velocityVariance);

    /**
     * Gets the initial velocity of new particles.
     *
     * @return The initial velocity of new particles.
     */
    const Vector3& getVelocity() const noexcept { return _velocity; }

    /**
     * Gets the initial velocity variance of new particles.
     *
     * @return The initial velocity variance of new particles.
     */
    const Vector3& getVelocityVariance() const noexcept { return _velocityVar; }

    /**
     * Gets the base acceleration vector of particles.
     *
     * @return The base acceleration vector of particles.
     */
    const Vector3& getAcceleration() const noexcept { return _acceleration; }

    /**
     * Sets the base acceleration vector and its allowed variance for this ParticleEmitter.
     *
     * @param acceleration The base acceleration vector of emitted particles.
     * @param accelerationVariance The variance allowed in the acceleration of emitted particles.
     */
    void setAcceleration(const Vector3& acceleration, const Vector3& accelerationVariance);

    /**
     * Gets the variance of acceleration of particles.
     *
     * @return The variance of acceleration of particles.
     */
    const Vector3& getAccelerationVariance() const noexcept { return _accelerationVar; }

    /**
     * Gets the maximum rotation speed of each emitted particle.
     * This determines the speed of rotation of each particle's screen-facing billboard.
     *
     * @param speedMin The minimum rotation speed (per particle).
     * @param speedMax The maximum rotation speed (per particle).
     */
    void setRotationPerParticle(float speedMin, float speedMax);

    /**
     * Gets the minimum rotation speed of each emitted particle.
     *
     * @return The minimum rotation speed of each emitted particle.
     */
    float getRotationPerParticleSpeedMin() const;

    /**
     * Gets the maximum rotation speed of each emitted particle.
     *
     * @return The maximum rotation speed of each emitted particle.
     */
    float getRotationPerParticleSpeedMax() const;

    /**
     * Sets a rotation axis in world space around which all particles will spin,
     * as well as the minimum and maximum rotation speed around this axis.
     * This should not be confused with rotation speed per particle.
     *
     * @param axis The base rotation axis of emitted particles.
     * @param axisVariance The variance of the rotation axis of emitted particles.
     * @param speedMin The minimum rotation speed of emitted particles.
     * @param speedMax The maximum rotation speed of emitted particles.
     */
    void setRotation(float speedMin, float speedMax, const Vector3& axis, const Vector3& axisVariance);

    /**
     * Gets the minimum rotation speed of emitted particles.
     *
     * @return The minimum rotation speed of emitted particles.
     */
    float getRotationSpeedMin() const noexcept { return _rotationSpeedMin; }

    /**
     * Gets the maximum rotation speed of emitted particles.
     *
     * @return The maximum rotation speed of emitted particles.
     */
    float getRotationSpeedMax() const noexcept { return _rotationSpeedMax; }

    /**
     * Gets the base rotation axis of emitted particles.
     *
     * @return The base rotation axis of emitted particles.
     */
    const Vector3& getRotationAxis() const noexcept { return _rotationAxis; }

    /**
     * Gets the variance of the rotation axis of emitted particles.
     *
     * @return The variance of the rotation axis of emitted particles.
     */
    const Vector3& getRotationAxisVariance() const noexcept { return _rotationAxisVar; }

    /**
     * Sets whether particles cycle through the sprite frames.
     *
     * @param animated Whether to animate particles through the sprite frames.
     */
    void setSpriteAnimated(bool animated) { _spriteAnimated = animated; }

    /**
     * Whether particles cycle through the sprite frames.
     */
    bool isSpriteAnimated() const noexcept { return _spriteAnimated; }

    /**
     * If sprites are set to loop, each frame will last for the emitter's frameDuration.
     * If sprites are set not to loop, the animation will be timed so that the last frame
     * finishes just as a particle dies.
     * Note: This timing is calculated based on a spriteRandomOffset of 0.
     * For other offsets, the final frame may be reached earlier.
     * If sprites are not set to animate, this setting has no effect.
     *
     * @param looped Whether to loop animated sprites.
     * @see ParticleEmitter::setSpriteFrameDuration
     */
    void setSpriteLooped(bool looped) { _spriteLooped = looped; }

    /**
     * Whether sprites are set to loop, each frame will last for the emitter's frameDuration.
     *
     * @return true if looped, false if not.
     */
    bool isSpriteLooped() const noexcept { return _spriteLooped; }

    /**
     * Sets the maximum offset that a random frame from 0 to maxOffset will be selected.
     * Set maxOffset to 0 (the default) for all particles to start on the first frame.
     * maxOffset will be clamped to frameCount.
     *
     * @param maxOffset The maximum sprite frame offset.
     */
    void setSpriteFrameRandomOffset(int maxOffset);

    /**
     * Gets the maximum offset that a random frame from 0 to maxOffset will be selected.
     */
    int getSpriteFrameRandomOffset() const noexcept { return _spriteFrameRandomOffset; }

    /**
     * Set the animated sprites frame duration.
     *
     * @param duration The duration of a single sprite frame, in milliseconds.
     */
    void setSpriteFrameDuration(long duration);

    /**
     * Gets the animated sprites frame duration.
     *
     * @return The animated sprites frame duration.
     */
    long getSpriteFrameDuration() const noexcept { return _spriteFrameDuration; }

    /**
     * Returns the width of the first frame this particle emitter's sprite.
     *
     * @return The width of the first frame of the sprite.
     */
    unsigned int getSpriteWidth() const;

    /**
     * Returns the height of the first frame this particle emitter's sprite.
     *
     * @return The height of the first frame of the sprite.
     */
    unsigned int getSpriteHeight() const;

    /**
     * Sets the sprite's texture coordinates in texture space.
     *
     * @param frameCount The number of frames to set texture coordinates for.
     * @param texCoords The texture coordinates for all frames, in texture space.
     */
    void setSpriteTexCoords(unsigned int frameCount, float* texCoords);

    /**
     * Sets the sprite's texture coordinates in image space (pixels).
     *
     * @param frameCount The number of frames to set texture coordinates for.
     * @param frameCoords A rectangle for each frame representing its position and size
     *  within the texture image, measured in pixels.
     */
    void setSpriteFrameCoords(unsigned int frameCount, Rectangle* frameCoords);

    /**
     * Calculates and sets the sprite's texture coordinates based on the width and
     * height of a single frame, measured in pixels.  This method assumes that there
     * is no padding between sprite frames and that the first frame is in the top-left
     * corner of the image.  Frames are ordered in the image from left to right, top to
     * bottom.
     *
     * @param frameCount The number of frames to set texture coordinates for.
     * @param width The width of a single frame, in pixels.
     * @param height The height of a single frame, in pixels.
     */
    void setSpriteFrameCoords(unsigned int frameCount, int width, int height);

    /**
     * Returns the current number of frames for the particle emitter's sprite.
     *
     * @return The current frame count.
     */
    unsigned int getSpriteFrameCount() const noexcept { return _spriteFrameCount; }

    /**
     * Sets whether the vector properties of newly emitted particles are rotated around the node's position
     * by the node's rotation matrix.
     *
     * @param orbitPosition Whether to rotate initial particle positions by the node's rotation matrix.
     * @param orbitVelocity Whether to rotate initial particle velocity vectors by the node's rotation matrix.
     * @param orbitAcceleration Whether to rotate initial particle acceleration vectors by the node's rotation matrix.
     */
    void setOrbit(bool orbitPosition, bool orbitVelocity, bool orbitAcceleration);

    /**
     * Whether new particle positions are rotated by the node's rotation matrix.
     *
     * @return True if orbiting positions, false otherwise.
     */
    bool getOrbitPosition() const noexcept { return _orbitPosition; }

    /**
     * Whether new particle velocities are rotated by the node's rotation matrix.
     *
     * @return True if orbiting velocities, false otherwise.
     */
    bool getOrbitVelocity() const noexcept { return _orbitVelocity; }

    /**
     * Whether new particle accelerations are rotated by the node's rotation matrix.
     *
     * @return True if orbiting accelerations, false otherwise.
     */
    bool getOrbitAcceleration() const noexcept { return _orbitAcceleration; }

    /**
     * Sets the texture blend mode for this particle emitter.
     *
     * @param blendMode The new blend mode.
     */
    void setBlendMode(BlendMode blendMode);

    /**
     * Gets the current texture blend mode for this particle emitter.
     *
     * @return The current blend mode.
     */
    BlendMode getBlendMode() const noexcept { return _spriteBlendMode; }

    /**
     * Updates the particles currently being emitted.
     *
     * @param elapsedTime The amount of time that has passed since the last call to update(), in milliseconds.
     */
    void update(float elapsedTime);

    /**
     * @see Drawable::draw
     *
     * Draws the particles currently being emitted.
     */
    unsigned int draw(bool wireframe = false);

  private:
    /**
     * Constructor.
     */
    explicit ParticleEmitter(unsigned int particlesCount);

    /**
     * Destructor.
     */
    ~ParticleEmitter();

    /**
     * @see Drawable::clone
     */
    Drawable* clone(NodeCloneContext& context);

    /**
     * Creates an uninitialized ParticleEmitter.
     *
     * @param texture the texture to use.
     * @param blendMode The blend mode to be used for the particles emitted.
     * @param particleCountMax The maximum number of particles that can be alive at one time in this
     * ParticleEmitter's system.
     * @script{create}
     */
    static ParticleEmitter* create(Texture* texture,
                                   BlendMode blendMode,
                                   unsigned int particleCountMax);

    /**
     * Hidden copy assignment operator.
     */
    ParticleEmitter& operator=(const ParticleEmitter&) = delete;

    // Generates a scalar within the range defined by min and max.
    float generateScalar(float min, float max);

    long generateScalar(long min, long max);

    // Generates a vector within the domain defined by a base vector and its variance.
    void generateVectorInRect(const Vector3& base, const Vector3& variance, Vector3* dst);

    // Generates a vector within the ellipsoidal domain defined by a center point and scale vector.
    void generateVectorInEllipsoid(const Vector3& center, const Vector3& scale, Vector3* dst);

    void generateVector(const Vector3& base, const Vector3& variance, Vector3* dst, bool ellipsoid);

    // Generates a color within the domain defined by a base vector and its variance.
    void generateColor(const Vector4& base, const Vector4& variance, Vector4* dst);

    // Gets the blend mode from string.
    static ParticleEmitter::BlendMode getBlendModeFromString(const std::string& src);

  private:
    /**
     * Defines the data for a single particle in the system.
     */
    class Particle
    {
      public:
        // 16-byte aligned Vector4 members (frequently accessed for rendering)
        // 16-byte aligned Vector4 members (frequently accessed for rendering)
        Vector4 _colorStart{ Vector4::one() }; // 16 bytes - default to white
        Vector4 _colorEnd{ Vector4::one() };   // 16 bytes - default to white
        Vector4 _color{ Vector4::one() };      // 16 bytes - default to white

        // 12-byte Vector3 members (physics/transform data)
        Vector3 _position{ Vector3::zero() };     // 12 bytes - default to origin
        Vector3 _velocity{ Vector3::zero() };     // 12 bytes - default to no movement
        Vector3 _acceleration{ Vector3::zero() }; // 12 bytes - default to no acceleration
        Vector3 _rotationAxis{ Vector3::zero() }; // 12 bytes - default to no rotation axis

        // 8-byte members (timing data)
        long _energyStart{ 1000L }; // 8 bytes - default to 1 second lifetime
        long _energy{ 1000L };      // 8 bytes - default to 1 second remaining

        // 4-byte members (size and rotation properties)
        float _sizeStart{ 1.0f };                // 4 bytes - default unit size
        float _sizeEnd{ 1.0f };                  // 4 bytes - default unit size
        float _size{ 1.0f };                     // 4 bytes - default unit size
        float _rotationPerParticleSpeed{ 0.0f }; // 4 bytes - default to no rotation
        float _rotationSpeed{ 0.0f };            // 4 bytes - default to no rotation
        float _angle{ 0.0f };                    // 4 bytes - default to no rotation
        float _timeOnCurrentFrame{ 0.0f };       // 4 bytes - default to frame start

        // 4-byte unsigned int (frame data)
        unsigned int _frame{ 0 }; // 4 bytes - default to first frame
    };

    static constexpr auto PARTICLE_COUNT_MAX = 100;
    static constexpr auto PARTICLE_UPDATE_RATE_MAX = 8;
    static constexpr auto PARTICLE_EMISSION_RATE = 10;
    static constexpr auto PARTICLE_EMISSION_RATE_TIME_INTERVAL =
        1000.0f / (float)PARTICLE_EMISSION_RATE;

    unsigned int _particleCountMax{ 0 };
    unsigned int _particleCount{ 0 };
    Particle* _particles{ nullptr };
    unsigned int _emissionRate{ PARTICLE_EMISSION_RATE };
    bool _started{ false };
    bool _ellipsoid{ false };
    float _sizeStartMin{ 1.0f };
    float _sizeStartMax{ 1.0f };
    float _sizeEndMin{ 1.0f };
    float _sizeEndMax{ 1.0f };
    float _energyMin{ 1000L };
    float _energyMax{ 1000L };
    Vector4 _colorStart{ Vector4::zero() };
    Vector4 _colorStartVar{ Vector4::zero() };
    Vector4 _colorEnd{ Vector4::one() };
    Vector4 _colorEndVar{ Vector4::zero() };
    Vector3 _position{ Vector3::zero() };
    Vector3 _positionVar{ Vector3::zero() };
    Vector3 _velocity{ Vector3::zero() };
    Vector3 _velocityVar{ Vector3::one() };
    Vector3 _acceleration{ Vector3::zero() };
    Vector3 _accelerationVar{ Vector3::zero() };
    float _rotationPerParticleSpeedMin{ 0.0f };
    float _rotationPerParticleSpeedMax{ 0.0f };
    float _rotationSpeedMin{ 0.0f };
    float _rotationSpeedMax{ 0.0f };
    Vector3 _rotationAxis{ Vector3::zero() };
    Vector3 _rotationAxisVar{ Vector3::zero() };
    Matrix _rotation{ Matrix::identity() };
    SpriteBatch* _spriteBatch{ nullptr };
    BlendMode _spriteBlendMode{ BLEND_ALPHA };
    float _spriteTextureWidth{ 0 };
    float _spriteTextureHeight{ 0 };
    float _spriteTextureWidthRatio{ 0 };
    float _spriteTextureHeightRatio{ 0 };
    float* _spriteTextureCoords{ nullptr };
    bool _spriteAnimated{ false };
    bool _spriteLooped{ false };
    unsigned int _spriteFrameCount{ 1 };
    unsigned int _spriteFrameRandomOffset{ 0 };
    long _spriteFrameDuration{ 0L };
    float _spriteFrameDurationSecs{ 0.0f };
    float _spritePercentPerFrame{ 0.0f };
    bool _orbitPosition{ false };
    bool _orbitVelocity{ false };
    bool _orbitAcceleration{ false };
    float _timePerEmission{ PARTICLE_EMISSION_RATE_TIME_INTERVAL };
    float _emitTime{ 0 };
    double _lastUpdated{ 0 };
};

} // namespace tractor

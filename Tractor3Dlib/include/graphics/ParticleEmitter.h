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

#include <memory>

#include "graphics/BlendMode.h"
#include "graphics/Drawable.h"
#include "graphics/IParticleAppearance.h"
#include "graphics/IParticleEmission.h"
#include "graphics/IParticlePhysics.h"
#include "graphics/IParticleSprite.h"
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
class ParticleEmitter;

/** Shared pointer type for ParticleEmitter. */
using ParticleEmitterPtr = std::shared_ptr<ParticleEmitter>;

/** Weak pointer type for ParticleEmitter. */
using ParticleEmitterWeakPtr = std::weak_ptr<ParticleEmitter>;

/**
 * Defines a particle emitter that can be made to simulate and render a particle system.
 *
 * This class implements the following segregated interfaces for clients that only need
 * specific functionality (Interface Segregation Principle):
 * - IParticleEmission: Start/stop emission, particle counts
 * - IParticlePhysics: Position, velocity, acceleration, rotation
 * - IParticleAppearance: Size, color, lifetime
 * - IParticleSprite: Texture, animation, blend mode
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
class ParticleEmitter : public Drawable,
                        public IParticleEmission,
                        public IParticlePhysics,
                        public IParticleAppearance,
                        public IParticleSprite
{
    friend class Node;

  public:
    /**
     * Type alias for backward compatibility.
     * @deprecated Use tractor::BlendMode enum class directly instead.
     */
    using BlendMode = tractor::BlendMode;

    /**
     * Backward-compatible blend mode constants.
     * @deprecated Use tractor::BlendMode enum class directly instead.
     */
    static constexpr tractor::BlendMode BLEND_NONE = tractor::BlendMode::NONE;
    static constexpr tractor::BlendMode BLEND_ALPHA = tractor::BlendMode::ALPHA;
    static constexpr tractor::BlendMode BLEND_ADDITIVE = tractor::BlendMode::ADDITIVE;
    static constexpr tractor::BlendMode BLEND_MULTIPLIED = tractor::BlendMode::MULTIPLIED;

    // Factory methods
    static ParticleEmitterPtr create(const std::string& url);
    static ParticleEmitterPtr create(Properties* properties);
    static ParticleEmitterPtr create(const std::string& texturePath,
                                     BlendMode blendMode,
                                     unsigned int particleCountMax);

    ~ParticleEmitter() = default;

    // ========================================================================
    // IParticleEmission interface implementation
    // ========================================================================
    void start() override;
    void stop() override { _started = false; }
    bool isStarted() const noexcept override { return _started; }
    bool isActive() const override;
    void emitOnce(unsigned int particleCount) override;
    unsigned int getParticlesCount() const noexcept override { return _particleCount; }
    void setEmissionRate(unsigned int rate) override;
    unsigned int getEmissionRate() const noexcept override { return _emissionRate; }
    void setParticleCountMax(unsigned int max) override { _particleCountMax = max; }
    unsigned int getParticleCountMax() const noexcept override { return _particleCountMax; }

    // ========================================================================
    // IParticlePhysics interface implementation
    // ========================================================================
    void setPosition(const Vector3& position, const Vector3& positionVariance) override;
    const Vector3& getPosition() const noexcept override { return _position; }
    const Vector3& getPositionVariance() const noexcept override { return _positionVar; }
    void setEllipsoid(bool ellipsoid) override { _ellipsoid = ellipsoid; }
    bool isEllipsoid() const noexcept override { return _ellipsoid; }

    void setVelocity(const Vector3& velocity, const Vector3& velocityVariance) override;
    const Vector3& getVelocity() const noexcept override { return _velocity; }
    const Vector3& getVelocityVariance() const noexcept override { return _velocityVar; }

    void setAcceleration(const Vector3& acceleration, const Vector3& accelerationVariance) override;
    const Vector3& getAcceleration() const noexcept override { return _acceleration; }
    const Vector3& getAccelerationVariance() const noexcept override { return _accelerationVar; }

    void setRotationPerParticle(float speedMin, float speedMax) override;
    float getRotationPerParticleSpeedMin() const override;
    float getRotationPerParticleSpeedMax() const override;

    void setRotation(float speedMin, float speedMax, 
                     const Vector3& axis, const Vector3& axisVariance) override;
    float getRotationSpeedMin() const noexcept override { return _rotationSpeedMin; }
    float getRotationSpeedMax() const noexcept override { return _rotationSpeedMax; }
    const Vector3& getRotationAxis() const noexcept override { return _rotationAxis; }
    const Vector3& getRotationAxisVariance() const noexcept override { return _rotationAxisVar; }

    void setOrbit(bool orbitPosition, bool orbitVelocity, bool orbitAcceleration) override;
    bool getOrbitPosition() const noexcept override { return _orbitPosition; }
    bool getOrbitVelocity() const noexcept override { return _orbitVelocity; }
    bool getOrbitAcceleration() const noexcept override { return _orbitAcceleration; }

    // ========================================================================
    // IParticleAppearance interface implementation
    // ========================================================================
    void setSize(float startMin, float startMax, float endMin, float endMax) override;
    float getSizeStartMin() const noexcept override { return _sizeStartMin; }
    float getSizeStartMax() const noexcept override { return _sizeStartMax; }
    float getSizeEndMin() const noexcept override { return _sizeEndMin; }
    float getSizeEndMax() const noexcept override { return _sizeEndMax; }

    void setColor(const Vector4& start, const Vector4& startVariance,
                  const Vector4& end, const Vector4& endVariance) override;
    const Vector4& getColorStart() const noexcept override { return _colorStart; }
    const Vector4& getColorStartVariance() const noexcept override { return _colorStartVar; }
    const Vector4& getColorEnd() const noexcept override { return _colorEnd; }
    const Vector4& getColorEndVariance() const noexcept override { return _colorEndVar; }

    void setEnergy(long energyMin, long energyMax) override;
    long getEnergyMin() const noexcept override { return _energyMin; }
    long getEnergyMax() const noexcept override { return _energyMax; }

    // ========================================================================
    // IParticleSprite interface implementation
    // ========================================================================
    void setTexture(const std::string& texturePath, BlendMode blendMode) override;
    void setTexture(Texture* texture, BlendMode blendMode) override;
    Texture* getTexture() const override;
    void setBlendMode(BlendMode blendMode) override;
    BlendMode getBlendMode() const noexcept override { return _spriteBlendMode; }

    void setSpriteAnimated(bool animated) override { _spriteAnimated = animated; }
    bool isSpriteAnimated() const noexcept override { return _spriteAnimated; }
    void setSpriteLooped(bool looped) override { _spriteLooped = looped; }
    bool isSpriteLooped() const noexcept override { return _spriteLooped; }
    void setSpriteFrameRandomOffset(int maxOffset) override;
    int getSpriteFrameRandomOffset() const noexcept override { return _spriteFrameRandomOffset; }
    void setSpriteFrameDuration(long duration) override;
    long getSpriteFrameDuration() const noexcept override { return _spriteFrameDuration; }

    unsigned int getSpriteWidth() const override;
    unsigned int getSpriteHeight() const override;
    void setSpriteTexCoords(unsigned int frameCount, float* texCoords) override;
    void setSpriteFrameCoords(unsigned int frameCount, Rectangle* frameCoords) override;
    void setSpriteFrameCoords(unsigned int frameCount, int width, int height) override;
    unsigned int getSpriteFrameCount() const noexcept override { return _spriteFrameCount; }

    // ========================================================================
    // Drawable interface and update
    // ========================================================================
    void update(float elapsedTime);
    unsigned int draw(bool wireframe = false) override;

    /**
     * Clones this particle emitter and returns a new instance as shared_ptr.
     *
     * @param context The clone context for tracking cloned objects.
     * @return The newly created particle emitter as shared_ptr.
     */
    DrawablePtr cloneDrawable(NodeCloneContext& context) const override;

  private:
    explicit ParticleEmitter(unsigned int particlesCount);
    
    /**
     * Legacy clone method - delegates to cloneDrawable().
     * @deprecated Use cloneDrawable() instead.
     */
    Drawable* clone(NodeCloneContext& context) override;

    static ParticleEmitterPtr create(Texture* texture,
                                     BlendMode blendMode,
                                     unsigned int particleCountMax);

    ParticleEmitter& operator=(const ParticleEmitter&) = delete;

    // Value generation helpers
    float generateScalar(float min, float max);
    long generateScalar(long min, long max);
    void generateVectorInRect(const Vector3& base, const Vector3& variance, Vector3* dst);
    void generateVectorInEllipsoid(const Vector3& center, const Vector3& scale, Vector3* dst);
    void generateVector(const Vector3& base, const Vector3& variance, Vector3* dst, bool ellipsoid);
    void generateColor(const Vector4& base, const Vector4& variance, Vector4* dst);

  private:
    class Particle
    {
      public:
        Vector4 _colorStart{ Vector4::one() };
        Vector4 _colorEnd{ Vector4::one() };
        Vector4 _color{ Vector4::one() };
        Vector3 _position{ Vector3::zero() };
        Vector3 _velocity{ Vector3::zero() };
        Vector3 _acceleration{ Vector3::zero() };
        Vector3 _rotationAxis{ Vector3::zero() };
        long _energyStart{ 1000L };
        long _energy{ 1000L };
        float _sizeStart{ 1.0f };
        float _sizeEnd{ 1.0f };
        float _size{ 1.0f };
        float _rotationPerParticleSpeed{ 0.0f };
        float _rotationSpeed{ 0.0f };
        float _angle{ 0.0f };
        float _timeOnCurrentFrame{ 0.0f };
        unsigned int _frame{ 0 };
    };

    static constexpr auto PARTICLE_COUNT_MAX = 100;
    static constexpr auto PARTICLE_UPDATE_RATE_MAX = 8;
    static constexpr auto PARTICLE_EMISSION_RATE = 10;
    static constexpr auto PARTICLE_EMISSION_RATE_TIME_INTERVAL =
        1000.0f / (float)PARTICLE_EMISSION_RATE;

    // Emission state
    unsigned int _particleCountMax{ 0 };
    unsigned int _particleCount{ 0 };
    std::unique_ptr<Particle[]> _particles;
    unsigned int _emissionRate{ PARTICLE_EMISSION_RATE };
    bool _started{ false };
    float _timePerEmission{ PARTICLE_EMISSION_RATE_TIME_INTERVAL };
    float _emitTime{ 0 };
    double _lastUpdated{ 0 };

    // Physics properties
    bool _ellipsoid{ false };
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
    bool _orbitPosition{ false };
    bool _orbitVelocity{ false };
    bool _orbitAcceleration{ false };

    // Appearance properties
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

    // Sprite properties
    std::unique_ptr<SpriteBatch> _spriteBatch;
    BlendMode _spriteBlendMode{ BlendMode::ALPHA };
    float _spriteTextureWidth{ 0 };
    float _spriteTextureHeight{ 0 };
    float _spriteTextureWidthRatio{ 0 };
    float _spriteTextureHeightRatio{ 0 };
    std::unique_ptr<float[]> _spriteTextureCoords;
    bool _spriteAnimated{ false };
    bool _spriteLooped{ false };
    unsigned int _spriteFrameCount{ 1 };
    unsigned int _spriteFrameRandomOffset{ 0 };
    long _spriteFrameDuration{ 0L };
    float _spriteFrameDurationSecs{ 0.0f };
    float _spritePercentPerFrame{ 0.0f };
};

} // namespace tractor

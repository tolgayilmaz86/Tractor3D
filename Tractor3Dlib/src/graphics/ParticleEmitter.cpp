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
#include "pch.h"

#include "graphics/ParticleEmitter.h"

#include "framework/Game.h"
#include "math/Quaternion.h"
#include "scene/Node.h"
#include "scene/Properties.h"
#include "scene/Scene.h"

namespace tractor
{

//----------------------------------------------------------------------------
ParticleEmitter::ParticleEmitter(unsigned int particleCountMax)
    : Drawable(), _particleCountMax(particleCountMax)
{
    _particles = new Particle[particleCountMax];
}

//----------------------------------------------------------------------------
ParticleEmitter::~ParticleEmitter()
{
    SAFE_DELETE(_spriteBatch);
    SAFE_DELETE_ARRAY(_particles);
    SAFE_DELETE_ARRAY(_spriteTextureCoords);
}

//----------------------------------------------------------------------------
ParticleEmitter* ParticleEmitter::create(const std::string& textureFile,
                                         BlendMode blendMode,
                                         unsigned int particleCountMax)
{
    Texture* texture = Texture::create(textureFile, true);

    if (!texture)
    {
        GP_ERROR("Failed to create texture for particle emitter.");
        return nullptr;
    }
    assert(texture->getWidth());
    assert(texture->getHeight());

    ParticleEmitter* emitter = ParticleEmitter::create(texture, blendMode, particleCountMax);
    SAFE_RELEASE(texture);
    return emitter;
}

//----------------------------------------------------------------------------
ParticleEmitter* ParticleEmitter::create(Texture* texture,
                                         BlendMode blendMode,
                                         unsigned int particleCountMax)
{
    ParticleEmitter* emitter = new ParticleEmitter(particleCountMax);
    assert(emitter);

    emitter->setTexture(texture, blendMode);

    return emitter;
}

//----------------------------------------------------------------------------
ParticleEmitter* ParticleEmitter::create(const std::string& url)
{
    auto properties = std::unique_ptr<Properties>(Properties::create(url));
    if (!properties)
    {
        GP_ERROR("Failed to create particle emitter from file.");
        return nullptr;
    }

    ParticleEmitter* particle =
        create(properties->getNamespace().length() > 0 ? properties.get() : properties->getNextNamespace());

    return particle;
}

//----------------------------------------------------------------------------
ParticleEmitter* ParticleEmitter::create(Properties* properties)
{
    if (!properties || properties->getNamespace() != "particle")
    {
        GP_ERROR("Properties object must be non-null and have namespace equal to 'particle'.");
        return nullptr;
    }
    properties->rewind();

    Properties* sprite = properties->getNextNamespace();
    if (!sprite || sprite->getNamespace() != "sprite")
    {
        GP_ERROR("Failed to load particle emitter: required namespace 'sprite' is missing.");
        return nullptr;
    }

    // Load sprite properties.
    // Path to image file is required.
    std::string texturePath;
    if (!sprite->getPath("path", texturePath))
    {
        GP_ERROR("Failed to load particle emitter: required image file path ('path') is missing.");
        return nullptr;
    }

    auto blendModeString = sprite->getString("blendMode");

    // Check for the old naming
    if (blendModeString.empty()) blendModeString = sprite->getString("blending");

    BlendMode blendMode = getBlendModeFromString(blendModeString);
    int spriteWidth = sprite->getInt("width");
    int spriteHeight = sprite->getInt("height");
    bool spriteAnimated = sprite->getBool("animated");
    bool spriteLooped = sprite->getBool("looped");
    int spriteFrameCount = sprite->getInt("frameCount");
    int spriteFrameRandomOffset = min(sprite->getInt("frameRandomOffset"), spriteFrameCount);
    float spriteFrameDuration = sprite->getFloat("frameDuration");

    // Emitter properties.
    unsigned int particleCountMax = (unsigned int)properties->getInt("particleCountMax");
    if (particleCountMax == 0)
    {
        // Set sensible default.
        particleCountMax = PARTICLE_COUNT_MAX;
    }

    unsigned int emissionRate = (unsigned int)properties->getInt("emissionRate");
    if (emissionRate == 0)
    {
        emissionRate = PARTICLE_EMISSION_RATE;
    }

    bool ellipsoid = properties->getBool("ellipsoid");
    float sizeStartMin = properties->getFloat("sizeStartMin");
    float sizeStartMax = properties->getFloat("sizeStartMax");
    float sizeEndMin = properties->getFloat("sizeEndMin");
    float sizeEndMax = properties->getFloat("sizeEndMax");
    long energyMin = properties->getLong("energyMin");
    long energyMax = properties->getLong("energyMax");

    Vector4 colorStart;
    Vector4 colorStartVar;
    Vector4 colorEnd;
    Vector4 colorEndVar;
    properties->getVector4("colorStart", &colorStart);
    properties->getVector4("colorStartVar", &colorStartVar);
    properties->getVector4("colorEnd", &colorEnd);
    properties->getVector4("colorEndVar", &colorEndVar);

    Vector3 position;
    Vector3 positionVar;
    Vector3 velocity;
    Vector3 velocityVar;
    Vector3 acceleration;
    Vector3 accelerationVar;
    Vector3 rotationAxis;
    Vector3 rotationAxisVar;
    properties->getVector3("position", &position);
    properties->getVector3("positionVar", &positionVar);
    properties->getVector3("velocity", &velocity);
    properties->getVector3("velocityVar", &velocityVar);
    properties->getVector3("acceleration", &acceleration);
    properties->getVector3("accelerationVar", &accelerationVar);
    float rotationPerParticleSpeedMin = properties->getFloat("rotationPerParticleSpeedMin");
    float rotationPerParticleSpeedMax = properties->getFloat("rotationPerParticleSpeedMax");
    float rotationSpeedMin = properties->getFloat("rotationSpeedMin");
    float rotationSpeedMax = properties->getFloat("rotationSpeedMax");
    properties->getVector3("rotationAxis", &rotationAxis);
    properties->getVector3("rotationAxisVar", &rotationAxisVar);
    bool orbitPosition = properties->getBool("orbitPosition");
    bool orbitVelocity = properties->getBool("orbitVelocity");
    bool orbitAcceleration = properties->getBool("orbitAcceleration");

    // Apply all properties to a newly created ParticleEmitter.
    ParticleEmitter* emitter = ParticleEmitter::create(texturePath, blendMode, particleCountMax);
    if (!emitter)
    {
        GP_ERROR("Failed to create particle emitter.");
        return nullptr;
    }
    emitter->setEmissionRate(emissionRate);
    emitter->setEllipsoid(ellipsoid);
    emitter->setSize(sizeStartMin, sizeStartMax, sizeEndMin, sizeEndMax);
    emitter->setEnergy(energyMin, energyMax);
    emitter->setColor(colorStart, colorStartVar, colorEnd, colorEndVar);
    emitter->setPosition(position, positionVar);
    emitter->setVelocity(velocity, velocityVar);
    emitter->setAcceleration(acceleration, accelerationVar);
    emitter->setRotationPerParticle(rotationPerParticleSpeedMin, rotationPerParticleSpeedMax);
    emitter->setRotation(rotationSpeedMin, rotationSpeedMax, rotationAxis, rotationAxisVar);
    emitter->setSpriteAnimated(spriteAnimated);
    emitter->setSpriteLooped(spriteLooped);
    emitter->setSpriteFrameRandomOffset(spriteFrameRandomOffset);
    emitter->setSpriteFrameDuration(spriteFrameDuration);
    emitter->setSpriteFrameCoords(spriteFrameCount, spriteWidth, spriteHeight);
    emitter->setOrbit(orbitPosition, orbitVelocity, orbitAcceleration);

    return emitter;
}

//----------------------------------------------------------------------------
void ParticleEmitter::setTexture(const std::string& texturePath, BlendMode blendMode)
{
    Texture* texture = Texture::create(texturePath, true);
    if (texture)
    {
        setTexture(texture, blendMode);
        texture->release();
    }
    else
    {
        GP_WARN("Failed set new texture on particle emitter: %s", texturePath);
    }
}

//----------------------------------------------------------------------------
void ParticleEmitter::setTexture(Texture* texture, BlendMode blendMode)
{
    // Create new batch before releasing old one, in case the same texture
    // is used for both (so it's not released before passing to the new batch).
    SpriteBatch* batch = SpriteBatch::create(texture, nullptr, _particleCountMax);
    batch->getSampler()->setFilterMode(Texture::LINEAR_MIPMAP_LINEAR, Texture::LINEAR);

    // Free existing batch
    SAFE_DELETE(_spriteBatch);

    _spriteBatch = batch;
    _spriteBatch->getStateBlock()->setDepthWrite(false);
    _spriteBatch->getStateBlock()->setDepthTest(true);

    setBlendMode(blendMode);
    _spriteTextureWidth = texture->getWidth();
    _spriteTextureHeight = texture->getHeight();
    _spriteTextureWidthRatio = 1.0f / (float)texture->getWidth();
    _spriteTextureHeightRatio = 1.0f / (float)texture->getHeight();

    // By default assume only one frame which uses the entire texture.
    Rectangle texCoord((float)texture->getWidth(), (float)texture->getHeight());
    setSpriteFrameCoords(1, &texCoord);
}

//----------------------------------------------------------------------------
Texture* ParticleEmitter::getTexture() const
{
    Texture::Sampler* sampler = _spriteBatch ? _spriteBatch->getSampler() : nullptr;
    return sampler ? sampler->getTexture() : nullptr;
}

//----------------------------------------------------------------------------
void ParticleEmitter::setEmissionRate(unsigned int rate)
{
    assert(rate);
    _emissionRate = rate;
    _timePerEmission = 1000.0f / (float)_emissionRate;
}

//----------------------------------------------------------------------------
void ParticleEmitter::start()
{
    _started = true;
    _lastUpdated = 0;
}

//----------------------------------------------------------------------------
bool ParticleEmitter::isActive() const
{
    if (_started) return true;

    if (!_node) return false;

    return (_particleCount > 0);
}

//----------------------------------------------------------------------------
void ParticleEmitter::emitOnce(unsigned int particleCount)
{
    assert(_node);
    assert(_particles);

    // Limit particleCount so as not to go over _particleCountMax.
    if (particleCount + _particleCount > _particleCountMax)
    {
        particleCount = _particleCountMax - _particleCount;
    }

    Vector3 translation;
    Matrix world = _node->getWorldMatrix();
    world.getTranslation(&translation);

    // Take translation out of world matrix so it can be used to rotate orbiting properties.
    world.m[12] = 0.0f;
    world.m[13] = 0.0f;
    world.m[14] = 0.0f;

    // Emit the new particles.
    for (size_t i = 0; i < particleCount; i++)
    {
        Particle* p = &_particles[_particleCount];

        generateColor(_colorStart, _colorStartVar, &p->_colorStart);
        generateColor(_colorEnd, _colorEndVar, &p->_colorEnd);
        p->_color.set(p->_colorStart);

        p->_energy = p->_energyStart = generateScalar(_energyMin, _energyMax);
        p->_size = p->_sizeStart = generateScalar(_sizeStartMin, _sizeStartMax);
        p->_sizeEnd = generateScalar(_sizeEndMin, _sizeEndMax);
        p->_rotationPerParticleSpeed =
            generateScalar(_rotationPerParticleSpeedMin, _rotationPerParticleSpeedMax);
        p->_angle = generateScalar(0.0f, p->_rotationPerParticleSpeed);
        p->_rotationSpeed = generateScalar(_rotationSpeedMin, _rotationSpeedMax);

        // Only initial position can be generated within an ellipsoidal domain.
        generateVector(_position, _positionVar, &p->_position, _ellipsoid);
        generateVector(_velocity, _velocityVar, &p->_velocity, false);
        generateVector(_acceleration, _accelerationVar, &p->_acceleration, false);
        generateVector(_rotationAxis, _rotationAxisVar, &p->_rotationAxis, false);

        // Initial position, velocity and acceleration can all be relative to the emitter's
        // transform. Rotate specified properties by the node's rotation.
        if (_orbitPosition)
        {
            world.transformPoint(p->_position, &p->_position);
        }

        if (_orbitVelocity)
        {
            world.transformPoint(p->_velocity, &p->_velocity);
        }

        if (_orbitAcceleration)
        {
            world.transformPoint(p->_acceleration, &p->_acceleration);
        }

        // The rotation axis always orbits the node.
        if (p->_rotationSpeed != 0.0f && !p->_rotationAxis.isZero())
        {
            world.transformPoint(p->_rotationAxis, &p->_rotationAxis);
        }

        // Translate position relative to the node's world space.
        p->_position.add(translation);

        // Initial sprite frame.
        if (_spriteFrameRandomOffset > 0)
        {
            p->_frame = rand() % _spriteFrameRandomOffset;
        }
        else
        {
            p->_frame = 0;
        }
        p->_timeOnCurrentFrame = 0.0f;

        ++_particleCount;
    }
}

//----------------------------------------------------------------------------
void ParticleEmitter::setSize(float startMin, float startMax, float endMin, float endMax)
{
    _sizeStartMin = startMin;
    _sizeStartMax = startMax;
    _sizeEndMin = endMin;
    _sizeEndMax = endMax;
}

//----------------------------------------------------------------------------
void ParticleEmitter::setEnergy(long energyMin, long energyMax)
{
    _energyMin = energyMin;
    _energyMax = energyMax;
}

//----------------------------------------------------------------------------
void ParticleEmitter::setColor(const Vector4& startColor,
                               const Vector4& startColorVar,
                               const Vector4& endColor,
                               const Vector4& endColorVar)
{
    _colorStart.set(startColor);
    _colorStartVar.set(startColorVar);
    _colorEnd.set(endColor);
    _colorEndVar.set(endColorVar);
}

void ParticleEmitter::setPosition(const Vector3& position, const Vector3& positionVar)
{
    _position.set(position);
    _positionVar.set(positionVar);
}

//----------------------------------------------------------------------------
void ParticleEmitter::setVelocity(const Vector3& velocity, const Vector3& velocityVar)
{
    _velocity.set(velocity);
    _velocityVar.set(velocityVar);
}

//----------------------------------------------------------------------------
void ParticleEmitter::setAcceleration(const Vector3& acceleration, const Vector3& accelerationVar)
{
    _acceleration.set(acceleration);
    _accelerationVar.set(accelerationVar);
}

//----------------------------------------------------------------------------
void ParticleEmitter::setRotationPerParticle(float speedMin, float speedMax)
{
    _rotationPerParticleSpeedMin = speedMin;
    _rotationPerParticleSpeedMax = speedMax;
}

//----------------------------------------------------------------------------
float ParticleEmitter::getRotationPerParticleSpeedMin() const
{
    return _rotationPerParticleSpeedMin;
}

//----------------------------------------------------------------------------
float ParticleEmitter::getRotationPerParticleSpeedMax() const
{
    return _rotationPerParticleSpeedMax;
}

//----------------------------------------------------------------------------
void ParticleEmitter::setRotation(float speedMin,
                                  float speedMax,
                                  const Vector3& axis,
                                  const Vector3& axisVariance)
{
    _rotationSpeedMin = speedMin;
    _rotationSpeedMax = speedMax;
    _rotationAxis.set(axis);
    _rotationAxisVar.set(axisVariance);
}

//----------------------------------------------------------------------------
void ParticleEmitter::setBlendMode(BlendMode blendMode)
{
    assert(_spriteBatch);
    assert(_spriteBatch->getStateBlock());

    switch (blendMode)
    {
        case BLEND_NONE:
            _spriteBatch->getStateBlock()->setBlend(false);
            break;
        case BLEND_ALPHA:
            _spriteBatch->getStateBlock()->setBlend(true);
            _spriteBatch->getStateBlock()->setBlendSrc(RenderState::BLEND_SRC_ALPHA);
            _spriteBatch->getStateBlock()->setBlendDst(RenderState::BLEND_ONE_MINUS_SRC_ALPHA);
            break;
        case BLEND_ADDITIVE:
            _spriteBatch->getStateBlock()->setBlend(true);
            _spriteBatch->getStateBlock()->setBlendSrc(RenderState::BLEND_SRC_ALPHA);
            _spriteBatch->getStateBlock()->setBlendDst(RenderState::BLEND_ONE);
            break;
        case BLEND_MULTIPLIED:
            _spriteBatch->getStateBlock()->setBlend(true);
            _spriteBatch->getStateBlock()->setBlendSrc(RenderState::BLEND_ZERO);
            _spriteBatch->getStateBlock()->setBlendDst(RenderState::BLEND_SRC_COLOR);
            break;
        default:
            GP_ERROR("Unsupported blend mode (%d).", blendMode);
            break;
    }

    _spriteBlendMode = blendMode;
}

//----------------------------------------------------------------------------
void ParticleEmitter::setSpriteFrameRandomOffset(int maxOffset)
{
    _spriteFrameRandomOffset = maxOffset;
}

//----------------------------------------------------------------------------
void ParticleEmitter::setSpriteFrameDuration(long duration)
{
    _spriteFrameDuration = duration;
    _spriteFrameDurationSecs = (float)duration / 1000.0f;
}

//----------------------------------------------------------------------------
unsigned int ParticleEmitter::getSpriteWidth() const
{
    return (unsigned int)fabs(_spriteTextureWidth
                              * (_spriteTextureCoords[2] - _spriteTextureCoords[0]));
}

//----------------------------------------------------------------------------
unsigned int ParticleEmitter::getSpriteHeight() const
{
    return (unsigned int)fabs(_spriteTextureHeight
                              * (_spriteTextureCoords[3] - _spriteTextureCoords[1]));
}

//----------------------------------------------------------------------------
void ParticleEmitter::setSpriteTexCoords(unsigned int frameCount, float* texCoords)
{
    assert(frameCount);
    assert(texCoords);

    _spriteFrameCount = frameCount;
    _spritePercentPerFrame = 1.0f / (float)frameCount;

    SAFE_DELETE_ARRAY(_spriteTextureCoords);
    _spriteTextureCoords = new float[frameCount * 4];
    memcpy(_spriteTextureCoords, texCoords, frameCount * 4 * sizeof(float));
}

//----------------------------------------------------------------------------
void ParticleEmitter::setSpriteFrameCoords(unsigned int frameCount, Rectangle* frameCoords)
{
    assert(frameCount);
    assert(frameCoords);

    _spriteFrameCount = frameCount;
    _spritePercentPerFrame = 1.0f / (float)frameCount;

    SAFE_DELETE_ARRAY(_spriteTextureCoords);
    _spriteTextureCoords = new float[frameCount * 4];

    // Pre-compute texture coordinates from rects.
    for (size_t i = 0; i < frameCount; i++)
    {
        _spriteTextureCoords[i * 4] = _spriteTextureWidthRatio * frameCoords[i].x;
        _spriteTextureCoords[i * 4 + 1] = 1.0f - _spriteTextureHeightRatio * frameCoords[i].y;
        _spriteTextureCoords[i * 4 + 2] =
            _spriteTextureCoords[i * 4] + _spriteTextureWidthRatio * frameCoords[i].width;
        _spriteTextureCoords[i * 4 + 3] =
            _spriteTextureCoords[i * 4 + 1] - _spriteTextureHeightRatio * frameCoords[i].height;
    }
}

//----------------------------------------------------------------------------
void ParticleEmitter::setSpriteFrameCoords(unsigned int frameCount, int width, int height)
{
    assert(width);
    assert(height);

    Rectangle* frameCoords = new Rectangle[frameCount];
    unsigned int cols = _spriteTextureWidth / width;
    unsigned int rows = _spriteTextureHeight / height;

    unsigned int n = 0;
    for (size_t i = 0; i < rows; ++i)
    {
        int y = i * height;
        for (size_t j = 0; j < cols; ++j)
        {
            int x = j * width;
            frameCoords[i * cols + j] = Rectangle(x, y, width, height);
            if (++n == frameCount)
            {
                break;
            }
        }

        if (n == frameCount)
        {
            break;
        }
    }

    setSpriteFrameCoords(frameCount, frameCoords);

    SAFE_DELETE_ARRAY(frameCoords);
}

//----------------------------------------------------------------------------
void ParticleEmitter::setOrbit(bool orbitPosition, bool orbitVelocity, bool orbitAcceleration)
{
    _orbitPosition = orbitPosition;
    _orbitVelocity = orbitVelocity;
    _orbitAcceleration = orbitAcceleration;
}

//----------------------------------------------------------------------------
long ParticleEmitter::generateScalar(long min, long max)
{
    // Note: this is not a very good RNG, but it should be suitable for our purposes.
    long r = 0;
    for (size_t i = 0; i < sizeof(long) / sizeof(int); i++)
    {
        r = r << 8; // sizeof(int) * CHAR_BITS
        r |= rand();
    }

    // Now we have a random long between 0 and MAX_LONG.  We need to clamp it between min and max.
    r %= max - min;
    r += min;

    return r;
}

//----------------------------------------------------------------------------
float ParticleEmitter::generateScalar(float min, float max)
{
    return min + (max - min) * MATH_RANDOM_0_1();
}

//----------------------------------------------------------------------------
void ParticleEmitter::generateVectorInRect(const Vector3& base, const Vector3& variance, Vector3* dst)
{
    assert(dst);

    // Scale each component of the variance vector by a random float
    // between -1 and 1, then add this to the corresponding base component.
    dst->x = base.x + variance.x * MATH_RANDOM_MINUS1_1();
    dst->y = base.y + variance.y * MATH_RANDOM_MINUS1_1();
    dst->z = base.z + variance.z * MATH_RANDOM_MINUS1_1();
}

//----------------------------------------------------------------------------
void ParticleEmitter::generateVectorInEllipsoid(const Vector3& center,
                                                const Vector3& scale,
                                                Vector3* dst)
{
    assert(dst);

    // Generate a point within a unit cube, then reject if the point is not in a unit sphere.
    do
    {
        dst->x = MATH_RANDOM_MINUS1_1();
        dst->y = MATH_RANDOM_MINUS1_1();
        dst->z = MATH_RANDOM_MINUS1_1();
    } while (dst->length() > 1.0f);

    // Scale this point by the scaling vector.
    dst->x *= scale.x;
    dst->y *= scale.y;
    dst->z *= scale.z;

    // Translate by the center point.
    dst->add(center);
}

//----------------------------------------------------------------------------
void ParticleEmitter::generateVector(const Vector3& base,
                                     const Vector3& variance,
                                     Vector3* dst,
                                     bool ellipsoid)
{
    if (ellipsoid)
    {
        generateVectorInEllipsoid(base, variance, dst);
    }
    else
    {
        generateVectorInRect(base, variance, dst);
    }
}

//----------------------------------------------------------------------------
void ParticleEmitter::generateColor(const Vector4& base, const Vector4& variance, Vector4* dst)
{
    assert(dst);

    // Scale each component of the variance color by a random float
    // between -1 and 1, then add this to the corresponding base component.
    dst->x = base.x + variance.x * MATH_RANDOM_MINUS1_1();
    dst->y = base.y + variance.y * MATH_RANDOM_MINUS1_1();
    dst->z = base.z + variance.z * MATH_RANDOM_MINUS1_1();
    dst->w = base.w + variance.w * MATH_RANDOM_MINUS1_1();
}

//----------------------------------------------------------------------------
ParticleEmitter::BlendMode ParticleEmitter::getBlendModeFromString(const std::string& str)
{
    if (str == "BLEND_NONE" || str == "NONE")
    {
        return BLEND_NONE;
    }
    if (str == "BLEND_OPAQUE" || str == "OPAQUE")
    {
        return BLEND_NONE;
    }
    if (str == "BLEND_ALPHA" || str == "ALPHA")
    {
        return BLEND_ALPHA;
    }
    if (str == "BLEND_TRANSPARENT" || str == "TRANSPARENT")
    {
        return BLEND_ALPHA;
    }
    if (str == "BLEND_ADDITIVE" || str == "ADDITIVE")
    {
        return BLEND_ADDITIVE;
    }
    if (str == "BLEND_MULTIPLIED" || str == "MULTIPLIED")
    {
        return BLEND_MULTIPLIED;
    }

    return BLEND_ALPHA;
}

//----------------------------------------------------------------------------
void ParticleEmitter::update(float elapsedTime)
{
    if (!isActive()) return;

    // Cap particle updates at a maximum rate. This saves processing
    // and also improves precision since updating with very small
    // time increments is more lossy.
    static double runningTime = 0;
    runningTime += elapsedTime;
    if (runningTime < PARTICLE_UPDATE_RATE_MAX) return;

    float elapsedMs = runningTime;
    runningTime = 0;

    float elapsedSecs = elapsedMs * 0.001f;

    if (_started && _emissionRate)
    {
        // Calculate how much time has passed since we last emitted particles.
        _emitTime += elapsedMs; //+= elapsedTime;

        // How many particles should we emit this frame?
        assert(_timePerEmission);
        unsigned int emitCount = (unsigned int)(_emitTime / _timePerEmission);

        if (emitCount)
        {
            if ((int)_timePerEmission > 0)
            {
                _emitTime = fmod((double)_emitTime, (double)_timePerEmission);
            }
            emitOnce(emitCount);
        }
    }

    // Now update all currently living particles.
    assert(_particles);
    for (size_t particlesIndex = 0; particlesIndex < _particleCount; ++particlesIndex)
    {
        Particle* p = &_particles[particlesIndex];
        p->_energy -= elapsedMs;

        if (p->_energy > 0L)
        {
            if (p->_rotationSpeed != 0.0f && !p->_rotationAxis.isZero())
            {
                _rotation = Matrix::createRotation(p->_rotationAxis, p->_rotationSpeed * elapsedSecs);

                _rotation.transformPoint(p->_velocity, &p->_velocity);
                _rotation.transformPoint(p->_acceleration, &p->_acceleration);
            }

            // Particle is still alive.
            p->_velocity.x += p->_acceleration.x * elapsedSecs;
            p->_velocity.y += p->_acceleration.y * elapsedSecs;
            p->_velocity.z += p->_acceleration.z * elapsedSecs;

            p->_position.x += p->_velocity.x * elapsedSecs;
            p->_position.y += p->_velocity.y * elapsedSecs;
            p->_position.z += p->_velocity.z * elapsedSecs;

            p->_angle += p->_rotationPerParticleSpeed * elapsedSecs;

            // Simple linear interpolation of color and size.
            float percent = 1.0f - ((float)p->_energy / (float)p->_energyStart);

            p->_color.x = p->_colorStart.x + (p->_colorEnd.x - p->_colorStart.x) * percent;
            p->_color.y = p->_colorStart.y + (p->_colorEnd.y - p->_colorStart.y) * percent;
            p->_color.z = p->_colorStart.z + (p->_colorEnd.z - p->_colorStart.z) * percent;
            p->_color.w = p->_colorStart.w + (p->_colorEnd.w - p->_colorStart.w) * percent;

            p->_size = p->_sizeStart + (p->_sizeEnd - p->_sizeStart) * percent;

            // Handle sprite animations.
            if (_spriteAnimated)
            {
                if (!_spriteLooped)
                {
                    // The last frame should finish exactly when the particle dies.
                    float percentSpent = 0.0f;
                    for (size_t i = 0; i < p->_frame; i++)
                    {
                        percentSpent += _spritePercentPerFrame;
                    }
                    p->_timeOnCurrentFrame = percent - percentSpent;
                    if (p->_frame < _spriteFrameCount - 1
                        && p->_timeOnCurrentFrame >= _spritePercentPerFrame)
                    {
                        ++p->_frame;
                    }
                }
                else
                {
                    // _spriteFrameDurationSecs is an absolute time measured in seconds,
                    // and the animation repeats indefinitely.
                    p->_timeOnCurrentFrame += elapsedSecs;
                    if (p->_timeOnCurrentFrame >= _spriteFrameDurationSecs)
                    {
                        p->_timeOnCurrentFrame -= _spriteFrameDurationSecs;
                        ++p->_frame;
                        if (p->_frame == _spriteFrameCount)
                        {
                            p->_frame = 0;
                        }
                    }
                }
            }
        }
        else
        {
            // Particle is dead.  Move the particle furthest from the start of the array
            // down to take its place, and re-use the slot at the end of the list of living particles.
            if (particlesIndex != _particleCount - 1)
            {
                _particles[particlesIndex] = _particles[_particleCount - 1];
            }
            --_particleCount;
        }
    }
}

//----------------------------------------------------------------------------
unsigned int ParticleEmitter::draw(bool wireframe)
{
    if (!isActive()) return 0;

    if (_particleCount > 0)
    {
        assert(_spriteBatch);
        assert(_particles);
        assert(_spriteTextureCoords);

        // Set our node's view projection matrix to this emitter's effect.
        if (_node)
        {
            _spriteBatch->setProjectionMatrix(_node->getViewProjectionMatrix());
        }

        // Begin sprite batch drawing
        _spriteBatch->start();

        // 2D Rotation.
        static const Vector2 pivot(0.5f, 0.5f);

        // 3D Rotation so that particles always face the camera.
        assert(_node && _node->getScene() && _node->getScene()->getActiveCamera()
               && _node->getScene()->getActiveCamera()->getNode());
        const Matrix& cameraWorldMatrix =
            _node->getScene()->getActiveCamera()->getNode()->getWorldMatrix();

        Vector3 right = cameraWorldMatrix.getRightVector();
        Vector3 up = cameraWorldMatrix.getUpVector();

        for (size_t i = 0; i < _particleCount; i++)
        {
            Particle* p = &_particles[i];

            _spriteBatch->draw(p->_position,
                               right,
                               up,
                               p->_size,
                               p->_size,
                               _spriteTextureCoords[p->_frame * 4],
                               _spriteTextureCoords[p->_frame * 4 + 1],
                               _spriteTextureCoords[p->_frame * 4 + 2],
                               _spriteTextureCoords[p->_frame * 4 + 3],
                               p->_color,
                               pivot,
                               p->_angle);
        }

        // Render.
        _spriteBatch->finish();
    }
    return 1;
}

//----------------------------------------------------------------------------
Drawable* ParticleEmitter::clone(NodeCloneContext& context)
{
    // Create a clone of this emitter
    ParticleEmitter* clone = ParticleEmitter::create(_spriteBatch->getSampler()->getTexture(),
                                                     _spriteBlendMode,
                                                     _particleCountMax);
    // Clone properties
    clone->setEmissionRate(_emissionRate);
    clone->_ellipsoid = _ellipsoid;
    clone->_sizeStartMin = _sizeStartMin;
    clone->_sizeStartMax = _sizeStartMax;
    clone->_sizeEndMin = _sizeEndMin;
    clone->_sizeEndMax = _sizeEndMax;
    clone->_energyMin = _energyMin;
    clone->_energyMax = _energyMax;
    clone->_colorStart = _colorStart;
    clone->_colorStartVar = _colorStartVar;
    clone->_colorEnd = _colorEnd;
    clone->_colorEndVar = _colorEndVar;
    clone->_position = _position;
    clone->_positionVar = _positionVar;
    clone->_velocity = _velocity;
    clone->_velocityVar = _velocityVar;
    clone->_acceleration = _acceleration;
    clone->_accelerationVar = _accelerationVar;
    clone->_rotationPerParticleSpeedMin = _rotationPerParticleSpeedMin;
    clone->_rotationPerParticleSpeedMax = _rotationPerParticleSpeedMax;
    clone->_rotationSpeedMin = _rotationSpeedMin;
    clone->_rotationSpeedMax = _rotationSpeedMax;
    clone->_rotationAxis = _rotationAxis;
    clone->_rotationAxisVar = _rotationAxisVar;
    clone->setSpriteTexCoords(_spriteFrameCount, _spriteTextureCoords);
    clone->_spriteAnimated = _spriteAnimated;
    clone->_spriteLooped = _spriteLooped;
    clone->_spriteFrameRandomOffset = _spriteFrameRandomOffset;
    clone->setSpriteFrameDuration(_spriteFrameDuration);
    clone->_orbitPosition = _orbitPosition;
    clone->_orbitVelocity = _orbitVelocity;
    clone->_orbitAcceleration = _orbitAcceleration;

    return clone;
}

} // namespace tractor

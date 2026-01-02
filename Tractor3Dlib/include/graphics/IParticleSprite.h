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

#include "graphics/BlendMode.h"
#include "graphics/Rectangle.h"

namespace tractor
{

class Texture;

/**
 * Interface for particle sprite/texture properties.
 * 
 * This interface follows the Interface Segregation Principle by separating
 * sprite animation and texture configuration from physics and emission concerns.
 * 
 * Clients that need to configure particle sprites (animation, frames, texture)
 * can depend on this interface without coupling to full ParticleEmitter functionality.
 */
class IParticleSprite
{
public:
    virtual ~IParticleSprite() = default;

    // Texture properties

    /**
     * Sets a new texture for this particle emitter.
     *
     * @param texturePath Path to the new texture.
     * @param blendMode Blend mode for the new texture.
     */
    virtual void setTexture(const std::string& texturePath, BlendMode blendMode) = 0;

    /**
     * Sets a new texture for this particle emitter.
     *
     * @param texture The new texture.
     * @param blendMode Blend mode for the new texture.
     */
    virtual void setTexture(Texture* texture, BlendMode blendMode) = 0;

    /**
     * Returns the texture currently set for this particle emitter.
     *
     * @return The current texture.
     */
    virtual Texture* getTexture() const = 0;

    /**
     * Sets the texture blend mode.
     *
     * @param blendMode The new blend mode.
     */
    virtual void setBlendMode(BlendMode blendMode) = 0;

    /**
     * Gets the current texture blend mode.
     *
     * @return The current blend mode.
     */
    virtual BlendMode getBlendMode() const noexcept = 0;

    // Animation properties

    /**
     * Sets whether particles cycle through sprite frames.
     *
     * @param animated Whether to animate particles.
     */
    virtual void setSpriteAnimated(bool animated) = 0;

    /**
     * Whether particles cycle through sprite frames.
     *
     * @return true if animated, false otherwise.
     */
    virtual bool isSpriteAnimated() const noexcept = 0;

    /**
     * Sets whether sprite animation loops.
     *
     * @param looped Whether to loop animated sprites.
     */
    virtual void setSpriteLooped(bool looped) = 0;

    /**
     * Whether sprite animation loops.
     *
     * @return true if looped, false otherwise.
     */
    virtual bool isSpriteLooped() const noexcept = 0;

    /**
     * Sets the maximum random frame offset for new particles.
     *
     * @param maxOffset The maximum sprite frame offset.
     */
    virtual void setSpriteFrameRandomOffset(int maxOffset) = 0;

    /**
     * Gets the maximum random frame offset.
     *
     * @return The maximum sprite frame offset.
     */
    virtual int getSpriteFrameRandomOffset() const noexcept = 0;

    /**
     * Sets the duration of a single sprite frame.
     *
     * @param duration The duration in milliseconds.
     */
    virtual void setSpriteFrameDuration(long duration) = 0;

    /**
     * Gets the duration of a single sprite frame.
     *
     * @return The duration in milliseconds.
     */
    virtual long getSpriteFrameDuration() const noexcept = 0;

    // Frame coordinate properties

    /**
     * Returns the width of the first frame.
     *
     * @return The width of the first frame.
     */
    virtual unsigned int getSpriteWidth() const = 0;

    /**
     * Returns the height of the first frame.
     *
     * @return The height of the first frame.
     */
    virtual unsigned int getSpriteHeight() const = 0;

    /**
     * Sets the sprite's texture coordinates in texture space.
     *
     * @param frameCount The number of frames.
     * @param texCoords The texture coordinates for all frames.
     */
    virtual void setSpriteTexCoords(unsigned int frameCount, float* texCoords) = 0;

    /**
     * Sets the sprite's texture coordinates in image space (pixels).
     *
     * @param frameCount The number of frames.
     * @param frameCoords A rectangle for each frame.
     */
    virtual void setSpriteFrameCoords(unsigned int frameCount, Rectangle* frameCoords) = 0;

    /**
     * Calculates and sets sprite texture coordinates from frame dimensions.
     *
     * @param frameCount The number of frames.
     * @param width The width of a single frame in pixels.
     * @param height The height of a single frame in pixels.
     */
    virtual void setSpriteFrameCoords(unsigned int frameCount, int width, int height) = 0;

    /**
     * Returns the current number of frames.
     *
     * @return The current frame count.
     */
    virtual unsigned int getSpriteFrameCount() const noexcept = 0;
};

} // namespace tractor

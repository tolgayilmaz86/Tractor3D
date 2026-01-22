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

#include <cstdint>
#include <string>

#include "renderer/RenderState.h"

namespace tractor
{

/**
 * Defines the types of blend modes used for rendering.
 * 
 * This enum consolidates the blend mode definitions that were previously
 * duplicated across ParticleEmitter, Sprite, and other rendering classes.
 */
enum class BlendMode : uint8_t
{
    NONE,       ///< No blending - opaque rendering
    ALPHA,      ///< Standard alpha blending (src_alpha, 1-src_alpha)
    ADDITIVE,   ///< Additive blending for glow effects (src_alpha, one)
    MULTIPLIED  ///< Multiplicative blending for shadows (zero, src_color)
};

/**
 * Utility class for blend mode operations.
 * 
 * Provides static methods for parsing blend modes from strings and
 * applying blend modes to render state blocks.
 */
class BlendModeUtils
{
public:
    /**
     * Parses a string to get the corresponding BlendMode enum value.
     *
     * Supported string values:
     * - "BLEND_NONE", "NONE", "BLEND_OPAQUE", "OPAQUE" -> BlendMode::NONE
     * - "BLEND_ALPHA", "ALPHA", "BLEND_TRANSPARENT", "TRANSPARENT" -> BlendMode::ALPHA
     * - "BLEND_ADDITIVE", "ADDITIVE" -> BlendMode::ADDITIVE
     * - "BLEND_MULTIPLIED", "MULTIPLIED" -> BlendMode::MULTIPLIED
     *
     * @param str The string to parse.
     * @param defaultMode The default mode to return if parsing fails.
     * @return The parsed BlendMode, or defaultMode if the string couldn't be parsed.
     */
    static BlendMode fromString(const std::string& str, BlendMode defaultMode = BlendMode::ALPHA);

    /**
     * Converts a BlendMode to its string representation.
     *
     * @param mode The blend mode to convert.
     * @return The string representation of the blend mode.
     */
    static std::string toString(BlendMode mode);

    /**
     * Applies the specified blend mode to a render state block.
     *
     * @param stateBlock The render state block to modify.
     * @param mode The blend mode to apply.
     */
    static void applyToStateBlock(RenderState::StateBlock* stateBlock, BlendMode mode);

private:
    BlendModeUtils() = delete; // Static utility class
};

} // namespace tractor

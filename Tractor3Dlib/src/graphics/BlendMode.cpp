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

#include "graphics/BlendMode.h"

namespace tractor
{

//----------------------------------------------------------------------------
BlendMode BlendModeUtils::fromString(const std::string& str, BlendMode defaultMode)
{
    if (str.empty())
        return defaultMode;

    if (str == "BLEND_NONE" || str == "NONE" || str == "BLEND_OPAQUE" || str == "OPAQUE")
        return BlendMode::NONE;

    if (str == "BLEND_ALPHA" || str == "ALPHA" || str == "BLEND_TRANSPARENT" || str == "TRANSPARENT")
        return BlendMode::ALPHA;

    if (str == "BLEND_ADDITIVE" || str == "ADDITIVE")
        return BlendMode::ADDITIVE;

    if (str == "BLEND_MULTIPLIED" || str == "MULTIPLIED")
        return BlendMode::MULTIPLIED;

    return defaultMode;
}

//----------------------------------------------------------------------------
std::string BlendModeUtils::toString(BlendMode mode)
{
    switch (mode)
    {
        case BlendMode::NONE:
            return "BLEND_NONE";
        case BlendMode::ALPHA:
            return "BLEND_ALPHA";
        case BlendMode::ADDITIVE:
            return "BLEND_ADDITIVE";
        case BlendMode::MULTIPLIED:
            return "BLEND_MULTIPLIED";
        default:
            return "BLEND_ALPHA";
    }
}

//----------------------------------------------------------------------------
void BlendModeUtils::applyToStateBlock(RenderState::StateBlock* stateBlock, BlendMode mode)
{
    if (!stateBlock)
        return;

    switch (mode)
    {
        case BlendMode::NONE:
            stateBlock->setBlend(false);
            break;
        case BlendMode::ALPHA:
            stateBlock->setBlend(true);
            stateBlock->setBlendSrc(RenderState::BLEND_SRC_ALPHA);
            stateBlock->setBlendDst(RenderState::BLEND_ONE_MINUS_SRC_ALPHA);
            break;
        case BlendMode::ADDITIVE:
            stateBlock->setBlend(true);
            stateBlock->setBlendSrc(RenderState::BLEND_SRC_ALPHA);
            stateBlock->setBlendDst(RenderState::BLEND_ONE);
            break;
        case BlendMode::MULTIPLIED:
            stateBlock->setBlend(true);
            stateBlock->setBlendSrc(RenderState::BLEND_ZERO);
            stateBlock->setBlendDst(RenderState::BLEND_SRC_COLOR);
            break;
    }
}

} // namespace tractor

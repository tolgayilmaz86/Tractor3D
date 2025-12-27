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

#include "renderer/RenderTarget.h"
#include <vector>
#include <memory>

namespace tractor
{

static std::vector<std::weak_ptr<RenderTarget>> __renderTargets;

//----------------------------------------------------------------------------
RenderTarget::RenderTarget(const std::string& id) : _id(id) {}

//----------------------------------------------------------------------------
RenderTarget::~RenderTarget()
{
    _texture.reset();
}

//----------------------------------------------------------------------------
std::shared_ptr<RenderTarget> RenderTarget::create(const std::string& id,
                                                   unsigned int width,
                                                   unsigned int height,
                                                   Texture::Format format)
{
    // Create a new texture with the given width.
    TexturePtr texture = Texture::create(format, width, height, nullptr, false);
    if (texture == nullptr)
    {
        GP_ERROR("Failed to create texture for render target.");
        return nullptr;
    }

    auto renderTarget = std::shared_ptr<RenderTarget>(new RenderTarget(id));
    renderTarget->_texture = texture;

    // Add to cache as weak_ptr
    __renderTargets.push_back(renderTarget);

    return renderTarget;
}

//----------------------------------------------------------------------------
std::shared_ptr<RenderTarget> RenderTarget::create(const std::string& id, Texture* texture)
{
    auto renderTarget = std::shared_ptr<RenderTarget>(new RenderTarget(id));

    // Try to get shared_ptr from the texture if it was created with shared_ptr
    try
    {
        renderTarget->_texture = texture->shared_from_this();
    }
    catch (const std::bad_weak_ptr&)
    {
        // If texture is not managed by shared_ptr, create a non-owning shared_ptr
        // This is for backward compatibility only
        renderTarget->_texture = TexturePtr(texture, [](Texture*) {});
    }

    // Add to cache as weak_ptr
    __renderTargets.push_back(renderTarget);

    return renderTarget;
}

//----------------------------------------------------------------------------
std::shared_ptr<RenderTarget> RenderTarget::getRenderTarget(const std::string& id) noexcept
{
    for (auto& weakTarget : __renderTargets)
    {
        if (auto target = weakTarget.lock())
        {
            if (id == target->getId())
            {
                return target;
            }
        }
    }

    return nullptr;
}

} // namespace tractor

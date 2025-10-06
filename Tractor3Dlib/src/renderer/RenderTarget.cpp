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

namespace tractor
{

static std::vector<RenderTarget*> __renderTargets;

//----------------------------------------------------------------------------
RenderTarget::RenderTarget(const std::string& id) : _id(id) {}

//----------------------------------------------------------------------------
RenderTarget::~RenderTarget()
{
    SAFE_RELEASE(_texture);

    // Remove ourself from the cache.
    std::vector<RenderTarget*>::iterator it =
        std::find(__renderTargets.begin(), __renderTargets.end(), this);
    if (it != __renderTargets.end())
        __renderTargets.erase(it);
}

//----------------------------------------------------------------------------
RenderTarget* RenderTarget::create(const std::string& id,
                                   unsigned int width,
                                   unsigned int height,
                                   Texture::Format format)
{
    // Create a new texture with the given width.
    Texture* texture = Texture::create(format, width, height, nullptr, false);
    if (texture == nullptr)
    {
        GP_ERROR("Failed to create texture for render target.");
        return nullptr;
    }

    RenderTarget* rt = create(id, texture);
    texture->release();

    return rt;
}

//----------------------------------------------------------------------------
RenderTarget* RenderTarget::create(const std::string& id, Texture* texture)
{
    const auto& renderTarget = __renderTargets.emplace_back(new RenderTarget(id));

    renderTarget->_texture = texture;
    renderTarget->_texture->addRef();

    return renderTarget;
}

//----------------------------------------------------------------------------
RenderTarget* RenderTarget::getRenderTarget(const std::string& id) noexcept
{
    if (auto it = std::ranges::find_if(__renderTargets,
                                       [&id](const auto& rt) { return id == rt->getId(); });
        it != __renderTargets.end())
    {
        return *it;
    }

    return nullptr;
}

} // namespace tractor

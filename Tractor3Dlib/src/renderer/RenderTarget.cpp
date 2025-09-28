#include "pch.h"

#include "renderer/RenderTarget.h"

namespace tractor
{

static std::vector<RenderTarget*> __renderTargets;

RenderTarget::RenderTarget(const std::string& id) : _id(id), _texture(nullptr) {}

RenderTarget::~RenderTarget()
{
    SAFE_RELEASE(_texture);

    // Remove ourself from the cache.
    std::vector<RenderTarget*>::iterator it =
        std::find(__renderTargets.begin(), __renderTargets.end(), this);
    if (it != __renderTargets.end())
    {
        __renderTargets.erase(it);
    }
}

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

RenderTarget* RenderTarget::create(const std::string& id, Texture* texture)
{
    const auto& renderTarget = __renderTargets.emplace_back(new RenderTarget(id));

    renderTarget->_texture = texture;
    renderTarget->_texture->addRef();

    return renderTarget;
}

RenderTarget* RenderTarget::getRenderTarget(const std::string& id)
{
    // Search the vector for a matching ID.
    for (const auto& dst : __renderTargets)
    {
        assert(dst);
        if (id == dst->getId())
        {
            return dst;
        }
    }

    return nullptr;
}

} // namespace tractor

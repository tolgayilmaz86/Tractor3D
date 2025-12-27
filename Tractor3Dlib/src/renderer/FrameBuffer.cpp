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

#include "renderer/FrameBuffer.h"

#include "framework/Game.h"

constexpr auto FRAMEBUFFER_ID_DEFAULT = "org.gameplay3d.framebuffer.default";

namespace tractor
{

unsigned int FrameBuffer::_maxRenderTargets = 0;
std::vector<FrameBufferPtr> FrameBuffer::_frameBuffers;
FrameBufferPtr FrameBuffer::_defaultFrameBuffer;
FrameBufferPtr FrameBuffer::_currentFrameBuffer;

//----------------------------------------------------------------------------
FrameBuffer::FrameBuffer(const std::string& id,
                         unsigned int width,
                         unsigned int height,
                         FrameBufferHandle handle)
    : _id(id), _handle(handle), _renderTargetCount(0)
{
}

//----------------------------------------------------------------------------
FrameBuffer::~FrameBuffer()
{
    // shared_ptr vector handles cleanup automatically
    _renderTargets.clear();

    // shared_ptr automatically handles cleanup
    _depthStencilTarget.reset();

    // Release GL resource.
    if (_handle) GL_ASSERT(glDeleteFramebuffers(1, &_handle));

    // Remove self from vector.
    auto it = std::find_if(_frameBuffers.begin(), _frameBuffers.end(),
        [this](const FrameBufferPtr& fb) { return fb.get() == this; });
    if (it != _frameBuffers.end())
    {
        _frameBuffers.erase(it);
    }
}

//----------------------------------------------------------------------------
void FrameBuffer::initialize()
{
    // Query the current/initial FBO handle and store is as out 'default' frame buffer.
    // On many platforms this will simply be the zero (0) handle, but this is not always the case.
    GLint fbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
    _defaultFrameBuffer = FrameBufferPtr(new FrameBuffer(FRAMEBUFFER_ID_DEFAULT, 0, 0, (FrameBufferHandle)fbo));
    _currentFrameBuffer = _defaultFrameBuffer;

    // Query the max supported color attachments. This glGet operation is not supported
    // on GL ES 2.x, so if the define does not exist, assume a value of 1.
#ifdef GL_MAX_COLOR_ATTACHMENTS
    GLint val;
    GL_ASSERT(glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &val));
    _maxRenderTargets = (unsigned int)std::max(1, val);
#else
    _maxRenderTargets = 1;
#endif
}

//----------------------------------------------------------------------------
void FrameBuffer::finalize()
{
    _currentFrameBuffer.reset();
    _defaultFrameBuffer.reset();
    _frameBuffers.clear();
}

//----------------------------------------------------------------------------
FrameBufferPtr FrameBuffer::create(const std::string& id) { return create(id, 0, 0); }

FrameBufferPtr FrameBuffer::create(const std::string& id,
                                   unsigned int width,
                                   unsigned int height,
                                   Texture::Format format)
{
    std::shared_ptr<RenderTarget> renderTarget = nullptr;
    if (width > 0 && height > 0)
    {
        // Create a default RenderTarget with same ID.
        renderTarget = RenderTarget::create(id, width, height, format);
        if (renderTarget == nullptr)
        {
            GP_ERROR("Failed to create render target for frame buffer.");
            return nullptr;
        }
    }

    // Create the frame buffer
    GLuint handle = 0;
    GL_ASSERT(glGenFramebuffers(1, &handle));

    auto frameBuffer = FrameBufferPtr(new FrameBuffer(id, width, height, handle));
    _frameBuffers.push_back(frameBuffer);

    // Initialize the render target vector with the correct size
    frameBuffer->_renderTargets.resize(_maxRenderTargets);

    if (renderTarget)
    {
        frameBuffer->setRenderTarget(renderTarget, 0);
    }

    return frameBuffer;
}

//----------------------------------------------------------------------------
FrameBufferPtr FrameBuffer::getFrameBuffer(const std::string& id)
{
    // Search the vector for a matching ID.
    if (auto it =
            std::ranges::find_if(_frameBuffers, [&id](const auto& rt) { return id == rt->getId(); });
        it != _frameBuffers.end())
    {
        return *it;
    }

    return nullptr;
}

//----------------------------------------------------------------------------
unsigned int FrameBuffer::getWidth() const
{
    if (_renderTargetCount > 0 && !_renderTargets.empty() && _renderTargets[0] != nullptr)
        return _renderTargets[0]->getWidth();

    return 0;
}

//----------------------------------------------------------------------------
unsigned int FrameBuffer::getHeight() const
{
    if (_renderTargetCount > 0 && !_renderTargets.empty() && _renderTargets[0] != nullptr)
        return _renderTargets[0]->getHeight();

    return 0;
}

//----------------------------------------------------------------------------
void FrameBuffer::setRenderTarget(std::shared_ptr<RenderTarget> target, unsigned int index)
{
    assert(!target
           || (target->getTexture() && target->getTexture()->getType() == Texture::TEXTURE_2D));

    // No change
    if (_renderTargets[index] == target) return;

    setRenderTarget(target, index, GL_TEXTURE_2D);
}

//----------------------------------------------------------------------------
void FrameBuffer::setRenderTarget(std::shared_ptr<RenderTarget> target, Texture::CubeFace face, unsigned int index)
{
    assert(face >= Texture::POSITIVE_X && face <= Texture::NEGATIVE_Z);
    assert(!target
           || (target->getTexture() && target->getTexture()->getType() == Texture::TEXTURE_CUBE));

    setRenderTarget(target, index, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face);
}

//----------------------------------------------------------------------------
void FrameBuffer::setRenderTarget(std::shared_ptr<RenderTarget> target, unsigned int index, GLenum textureTarget)
{
    assert(index < _maxRenderTargets);
    assert(!_renderTargets.empty());

    // Release our reference to the current RenderTarget at this index.
    if (_renderTargets[index])
    {
        _renderTargets[index].reset();
        --_renderTargetCount;
    }

    _renderTargets[index] = target;

    if (target)
    {
        ++_renderTargetCount;

        // Now set this target as the color attachment corresponding to index.
        GL_ASSERT(glBindFramebuffer(GL_FRAMEBUFFER, _handle));
        GLenum attachment;
        if (target->getTexture()->getFormat() == Texture::DEPTH)
        {
            attachment = GL_DEPTH_ATTACHMENT;
            GL_ASSERT(glFramebufferTexture2D(GL_FRAMEBUFFER,
                                             attachment,
                                             textureTarget,
                                             _renderTargets[index]->getTexture()->getHandle(),
                                             0));
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
        }
        else
        {
            attachment = GL_COLOR_ATTACHMENT0 + index;
            GL_ASSERT(glFramebufferTexture2D(GL_FRAMEBUFFER,
                                             attachment,
                                             textureTarget,
                                             _renderTargets[index]->getTexture()->getHandle(),
                                             0));
        }

        GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
        {
            GP_ERROR("Framebuffer status incomplete: 0x%x", fboStatus);
        }

        // Restore the FBO binding
        GL_ASSERT(glBindFramebuffer(GL_FRAMEBUFFER, _currentFrameBuffer->_handle));
    }
}

//----------------------------------------------------------------------------
std::shared_ptr<RenderTarget> FrameBuffer::getRenderTarget(unsigned int index) const
{
    if (index < _renderTargets.size())
    {
        return _renderTargets[index];
    }
    return nullptr;
}

//----------------------------------------------------------------------------
void FrameBuffer::setDepthStencilTarget(std::shared_ptr<DepthStencilTarget> target)
{
    if (_depthStencilTarget == target) return;

    // shared_ptr assignment handles the reference counting automatically
    _depthStencilTarget = target;

    if (target)
    {
        // Now set this target as the color attachment corresponding to index.
        GL_ASSERT(glBindFramebuffer(GL_FRAMEBUFFER, _handle));

        // Attach the render buffer to the framebuffer
        GL_ASSERT(glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                            GL_DEPTH_ATTACHMENT,
                                            GL_RENDERBUFFER,
                                            _depthStencilTarget->_depthBuffer));
        if (target->isPacked())
        {
            GL_ASSERT(glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                                GL_STENCIL_ATTACHMENT,
                                                GL_RENDERBUFFER,
                                                _depthStencilTarget->_depthBuffer));
        }
        else if (target->getFormat() == DepthStencilTarget::DEPTH_STENCIL)
        {
            GL_ASSERT(glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                                GL_STENCIL_ATTACHMENT,
                                                GL_RENDERBUFFER,
                                                _depthStencilTarget->_stencilBuffer));
        }

        // Check the framebuffer is good to go.
        GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
        {
            GP_ERROR("Framebuffer status incomplete: 0x%x", fboStatus);
        }

        // Restore the FBO binding
        GL_ASSERT(glBindFramebuffer(GL_FRAMEBUFFER, _currentFrameBuffer->_handle));
    }
}

//----------------------------------------------------------------------------
FrameBufferPtr FrameBuffer::bind(GLenum type)
{
    GL_ASSERT(glBindFramebuffer(type, _handle));
    FrameBufferPtr previousFrameBuffer = _currentFrameBuffer;
    _currentFrameBuffer = shared_from_this();
    return previousFrameBuffer;
}

//----------------------------------------------------------------------------
void FrameBuffer::getScreenshot(const std::shared_ptr<Image>& image)
{
    assert(image);

    unsigned int width = _currentFrameBuffer->getWidth();
    unsigned int height = _currentFrameBuffer->getHeight();

    if (image->getWidth() == width && image->getHeight() == height)
    {
        GLenum format = image->getFormat() == Image::RGB ? GL_RGB : GL_RGBA;
        GL_ASSERT(glReadPixels(0, 0, width, height, format, GL_UNSIGNED_BYTE, image->getData()));
    }
}

//----------------------------------------------------------------------------
std::shared_ptr<Image> FrameBuffer::createScreenshot(Image::Format format)
{
    auto screenshot = Image::create(_currentFrameBuffer->getWidth(),
                                    _currentFrameBuffer->getHeight(),
                                    format,
                                    nullptr);
    getScreenshot(screenshot);

    return screenshot;
}

//----------------------------------------------------------------------------
FrameBufferPtr FrameBuffer::bindDefault(GLenum type)
{
    GL_ASSERT(glBindFramebuffer(type, _defaultFrameBuffer->_handle));
    _currentFrameBuffer = _defaultFrameBuffer;
    return _defaultFrameBuffer;
}

} // namespace tractor

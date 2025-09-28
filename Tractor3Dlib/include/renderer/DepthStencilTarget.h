#pragma once

#include "pch.h"

#include "renderer/Texture.h"

namespace tractor
{

/**
 * Defines a container for depth and stencil targets in a frame buffer object.
 */
class DepthStencilTarget : public Ref
{
    friend class FrameBuffer;

  public:
    /**
     * Defines the accepted formats for DepthStencilTargets.
     */
    enum Format
    {
        /**
         * A target with depth data.
         */
        DEPTH,

        /**
         * A target with depth data and stencil data.
         */
        DEPTH_STENCIL
    };

    /**
     * Create a DepthStencilTarget and add it to the list of available DepthStencilTargets.
     *
     * @param id The ID of the new DepthStencilTarget.  Uniqueness is recommended but not enforced.
     * @param format The format of the new DepthStencilTarget.
     * @param width Width of the new DepthStencilTarget.
     * @param height Height of the new DepthStencilTarget.
     *
     * @return A newly created DepthStencilTarget.
     * @script{create}
     */
    static DepthStencilTarget* create(const std::string& id,
                                      Format format,
                                      unsigned int width,
                                      unsigned int height);

    /**
     * Get a named DepthStencilTarget from its ID.
     *
     * @param id The ID of the DepthStencilTarget to search for.
     *
     * @return The DepthStencilTarget with the specified ID, or nullptr if one was not found.
     */
    static DepthStencilTarget* getDepthStencilTarget(const std::string& id);

    /**
     * Get the ID of this DepthStencilTarget.
     *
     * @return The ID of this DepthStencilTarget.
     */
    const std::string& getId() const noexcept { return _id; }

    /**
     * Returns the format of the DepthStencilTarget.
     *
     * @return The format.
     */
    DepthStencilTarget::Format getFormat() const noexcept { return _format; }

    /**
     * Returns the width of the DepthStencilTarget.
     *
     * @return The width.
     */
    unsigned int getWidth() const noexcept { return _width; }

    /**
     * Returns the height of the DepthStencilTarget.
     *
     * @return The height.
     */
    unsigned int getHeight() const noexcept { return _height; }

    /**
     * Returns true if depth and stencil buffer are packed.
     *
     * @return The packed state.
     */
    bool isPacked() const noexcept { return _packed; }

  private:
    /**
     * Constructor.
     */
    DepthStencilTarget(const std::string& id, Format format, unsigned int width, unsigned int height);

    /**
     * Destructor.
     */
    ~DepthStencilTarget();

    /**
     * Hidden copy assignment operator.
     */
    DepthStencilTarget& operator=(const DepthStencilTarget&) = delete;

    std::string _id;
    Format _format;
    RenderBufferHandle _depthBuffer;
    RenderBufferHandle _stencilBuffer;
    unsigned int _width;
    unsigned int _height;
    bool _packed;
};

} // namespace tractor

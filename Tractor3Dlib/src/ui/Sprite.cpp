#include "pch.h"

#include "graphics/Sprite.h"

#include "scene/Scene.h"

namespace
{
using namespace tractor;
/**
 * Parses a string to get the corresponding sprite offset enum value.
 *
 * @param str The string to parse.
 * @param offset The sprite offset enum value that is set based on the parsed string.
 * @return true if the string was successfully parsed; false if the string is empty or
 *         could not be parsed.
 */
static bool parseOffset(const std::string& str, Sprite::Offset* offset)
{
    assert(offset);

    if (str.empty())
    {
        *offset = Sprite::OFFSET_LEFT;
        return false;
    }

    if (str == "OFFSET_LEFT")
    {
        *offset = Sprite::OFFSET_LEFT;
    }
    else if (str == "OFFSET_HCENTER")
    {
        *offset = Sprite::OFFSET_HCENTER;
    }
    else if (str == "OFFSET_RIGHT")
    {
        *offset = Sprite::OFFSET_RIGHT;
    }
    else if (str == "OFFSET_TOP")
    {
        *offset = Sprite::OFFSET_TOP;
    }
    else if (str == "OFFSET_VCENTER")
    {
        *offset = Sprite::OFFSET_VCENTER;
    }
    else if (str == "OFFSET_BOTTOM")
    {
        *offset = Sprite::OFFSET_BOTTOM;
    }
    else if (str == "OFFSET_ANCHOR")
    {
        *offset = Sprite::OFFSET_ANCHOR;
    }
    else if (str == "OFFSET_TOP_LEFT")
    {
        *offset = Sprite::OFFSET_TOP_LEFT;
    }
    else if (str == "OFFSET_VCENTER_LEFT")
    {
        *offset = Sprite::OFFSET_VCENTER_LEFT;
    }
    else if (str == "OFFSET_BOTTOM_LEFT")
    {
        *offset = Sprite::OFFSET_BOTTOM_LEFT;
    }
    else if (str == "OFFSET_TOP_HCENTER")
    {
        *offset = Sprite::OFFSET_TOP_HCENTER;
    }
    else if (str == "OFFSET_VCENTER_HCENTER")
    {
        *offset = Sprite::OFFSET_VCENTER_HCENTER;
    }
    else if (str == "OFFSET_BOTTOM_HCENTER")
    {
        *offset = Sprite::OFFSET_BOTTOM_HCENTER;
    }
    else if (str == "OFFSET_TOP_RIGHT")
    {
        *offset = Sprite::OFFSET_TOP_RIGHT;
    }
    else if (str == "OFFSET_VCENTER_RIGHT")
    {
        *offset = Sprite::OFFSET_VCENTER_RIGHT;
    }
    else if (str == "OFFSET_BOTTOM_RIGHT")
    {
        *offset = Sprite::OFFSET_BOTTOM_RIGHT;
    }
    else
    {
        GP_ERROR("Failed to get corresponding sprite offset for unsupported value '%s'.",
                 str.c_str());
        *offset = Sprite::OFFSET_LEFT;
        return false;
    }

    return true;
}

/**
 * Parses a string to get the corresponding sprite blend mode enum value.
 *
 * @param str The string to parse.
 * @param blend The sprite blend mode enum value that is set based on the parsed string.
 * @return true if the string was successfully parsed; false if the string is empty or
 *         could not be parsed.
 */
static bool parseBlendMode(const std::string& str, Sprite::BlendMode* blend)
{
    assert(blend);

    if (str.empty())
    {
        *blend = Sprite::BLEND_NONE;
        return false;
    }

    if (str == "BLEND_ALPHA")
    {
        *blend = Sprite::BLEND_ALPHA;
    }
    else if (str == "BLEND_ADDITIVE")
    {
        *blend = Sprite::BLEND_ADDITIVE;
    }
    else if (str == "BLEND_MULTIPLIED")
    {
        *blend = Sprite::BLEND_MULTIPLIED;
    }
    else if (str != "BLEND_NONE")
    {
        GP_ERROR("Failed to get corresponding sprite blend mode for unsupported value '%s'.",
                 str.c_str());
        *blend = Sprite::BLEND_NONE;
        return false;
    }
    else
    {
        *blend = Sprite::BLEND_NONE;
    }

    return true;
}

/**
 * Parses a string to get the corresponding sprite flip flags enum value.
 *
 * @param str The string to parse.
 * @param flip The sprite flip flags enum value that is set based on the parsed string.
 * @return true if the string was successfully parsed; false if the string is empty or
 *         could not be parsed.
 */
static bool parseFlipFlags(const std::string& str, Sprite::FlipFlags* flip)
{
    assert(flip);

    if (str.empty())
    {
        *flip = Sprite::FLIP_NONE;
        return false;
    }

    if (str == "FLIP_VERTICAL")
    {
        *flip = Sprite::FLIP_VERTICAL;
    }
    else if (str == "FLIP_HORIZONTAL")
    {
        *flip = Sprite::FLIP_HORIZONTAL;
    }
    else if (str == "FLIP_VERTICAL_HORIZONTAL")
    {
        *flip = (Sprite::FlipFlags)(Sprite::FLIP_VERTICAL | Sprite::FLIP_HORIZONTAL);
    }
    else if (str != "FLIP_NONE")
    {
        GP_ERROR("Failed to get corresponding sprite flip flag for unsupported value '%s'.",
                 str.c_str());
        *flip = Sprite::FLIP_NONE;
        return false;
    }
    else
    {
        *flip = Sprite::FLIP_NONE;
    }

    return true;
}
} // namespace

namespace tractor
{

Sprite* Sprite::create(const std::string& imagePath, float width, float height, Effect* effect)
{
    return Sprite::create(imagePath, width, height, Rectangle(0, 0, -1, -1), 1, effect);
}

Sprite* Sprite::create(const std::string& imagePath,
                       float width,
                       float height,
                       const Rectangle& source,
                       unsigned int frameCount,
                       Effect* effect)
{
    assert(width >= -1 && height >= -1);
    assert(source.width >= -1 && source.height >= -1);
    assert(frameCount > 0);

    SpriteBatch* batch = SpriteBatch::create(imagePath, effect);
    batch->getSampler()->setWrapMode(Texture::CLAMP, Texture::CLAMP);
    batch->getSampler()->setFilterMode(Texture::Filter::LINEAR, Texture::Filter::LINEAR);
    batch->getStateBlock()->setDepthWrite(false);
    batch->getStateBlock()->setDepthTest(true);

    unsigned int imageWidth = batch->getSampler()->getTexture()->getWidth();
    unsigned int imageHeight = batch->getSampler()->getTexture()->getHeight();
    if (width == -1) width = imageWidth;
    if (height == -1) height = imageHeight;

    Sprite* sprite = new Sprite();
    sprite->_width = width;
    sprite->_height = height;
    sprite->_batch = std::shared_ptr<SpriteBatch>(batch);

    sprite->_frameCount = frameCount;
    sprite->_frames = std::make_unique<Rectangle[]>(frameCount);
    sprite->_frames[0] = source;
    if (sprite->_frames[0].width == -1.0f) sprite->_frames[0].width = imageWidth;
    if (sprite->_frames[0].height == -1.0f) sprite->_frames[0].height = imageHeight;

    return sprite;
}

Sprite* Sprite::create(Properties* properties)
{
    // Check if the Properties is valid and has a valid namespace.
    if (!properties || properties->getNamespace() != "sprite")
    {
        GP_ERROR("Properties object must be non-null and have namespace equal to 'sprite'.");
        return nullptr;
    }

    // Get image path.
    auto imagePath = properties->getString("path");
    if (imagePath.empty())
    {
        GP_ERROR("Sprite is missing required image file path.");
        return nullptr;
    }

    // Don't support loading custom effects
    Effect* effect = nullptr;

    // Get width and height
    float width = -1.0f;
    float height = -1.0f;
    float widthPercentage = 0.0f;
    float heightPercentage = 0.0f;
    if (properties->exists("width"))
    {
        if (properties->getType("width") == Properties::NUMBER)
        {
            // Number only (200)
            width = properties->getFloat("width");
        }
        else
        {
            // Number and something else (200%)
            widthPercentage = properties->getFloat("width") / 100.0f;
        }
    }
    if (properties->exists("height"))
    {
        if (properties->getType("height") == Properties::NUMBER)
        {
            height = properties->getFloat("height");
        }
        else
        {
            heightPercentage = properties->getFloat("height") / 100.0f;
        }
    }

    Sprite* sprite;
    if (properties->exists("source"))
    {
        // Get source frame
        Rectangle source;
        properties->getVector4("source", (Vector4*)&source);

        // Get frame count
        int frameCount = properties->getInt("frameCount");
        if (frameCount < 0)
        {
            GP_WARN("Sprites require at least one frame. Defaulting to frame count 1.");
        }
        if (frameCount < 1)
        {
            frameCount = 1;
        }

        // Load sprite
        sprite = Sprite::create(imagePath, width, height, source, frameCount, effect);
    }
    else
    {
        // Load sprite
        sprite = Sprite::create(imagePath, width, height, effect);
    }

    // Edit scaling of sprites if needed
    if (widthPercentage != 0.0f || heightPercentage != 0.0f)
    {
        if (widthPercentage != 0.0f)
        {
            sprite->_width *= widthPercentage;
            sprite->_frames[0].width *= widthPercentage;
        }
        if (heightPercentage != 0.0f)
        {
            sprite->_height *= heightPercentage;
            sprite->_frames[0].height *= heightPercentage;
        }
    }

    // Get anchor
    Vector4 vect;
    if (properties->getVector2("anchor", (Vector2*)&vect))
    {
        sprite->setAnchor(*((Vector2*)&vect));
    }

    // Get color
    if (properties->exists("color"))
    {
        switch (properties->getType("color"))
        {
            case Properties::VECTOR3:
                vect.w = 1.0f;
                properties->getVector3("color", (Vector3*)&vect);
                break;
            case Properties::VECTOR4:
                properties->getVector4("color", &vect);
                break;
            case Properties::STRING:
            default:
                properties->getColor("color", &vect);
                break;
        }
        sprite->setColor(vect);
    }

    // Get opacity
    if (properties->exists("opacity"))
    {
        sprite->setOpacity(properties->getFloat("opacity"));
    }

    // Get blend mode
    BlendMode mode;
    if (parseBlendMode(properties->getString("blendMode"), &mode))
    {
        sprite->setBlendMode(mode);
    }

    // Get flip flags
    FlipFlags flags;
    if (parseFlipFlags(properties->getString("flip"), &flags))
    {
        sprite->setFlip(flags);
    }

    // Get sprite offset
    Offset offset;
    if (parseOffset(properties->getString("offset"), &offset))
    {
        sprite->setOffset(offset);
    }

    return sprite;
}

void Sprite::setFrameSource(unsigned int frameIndex, const Rectangle& source)
{
    assert(frameIndex < _frameCount);

    _frames[frameIndex] = source;
}

const Rectangle& Sprite::getFrameSource(unsigned int frameIndex) const
{
    assert(frameIndex < _frameCount);

    return _frames[frameIndex];
}

void Sprite::computeFrames(unsigned int frameStride, unsigned int framePadding)
{
    _frameStride = frameStride;
    _framePadding = framePadding;

    if (_frameCount < 2) return;
    unsigned int imageWidth = _batch->getSampler()->getTexture()->getWidth();
    unsigned int imageHeight = _batch->getSampler()->getTexture()->getHeight();
    float textureWidthRatio = 1.0f / imageWidth;
    float textureHeightRatio = 1.0f / imageHeight;

    // If we have a stride then compute the wrap width
    float strideWidth;
    if (_frameStride > 0)
        strideWidth = _frameStride * _frames[0].width;
    else
        strideWidth = imageWidth;

    // Mark the start as reference
    float x = _frames[0].x;
    float y = _frames[0].y;
    float width = _frames[0].width;
    float height = _frames[0].height;

    // Compute frames 1+
    for (size_t frameIndex = 1; frameIndex < _frameCount; frameIndex++)
    {
        _frames[frameIndex].x = x;
        _frames[frameIndex].y = y;
        _frames[frameIndex].width = _width;
        _frames[frameIndex].height = _height;

        x += _frames[frameIndex].width + (float)_framePadding;
        if (x >= imageWidth)
        {
            y += _frames[frameIndex].height + (float)_framePadding;
            if (y >= imageHeight)
            {
                y = 0.0f;
            }
            x = 0.0f;
        }
    }
}

void Sprite::setBlendMode(BlendMode mode)
{
    switch (mode)
    {
        case BLEND_NONE:
            _batch->getStateBlock()->setBlend(false);
            break;
        case BLEND_ALPHA:
            _batch->getStateBlock()->setBlend(true);
            _batch->getStateBlock()->setBlendSrc(RenderState::BLEND_SRC_ALPHA);
            _batch->getStateBlock()->setBlendDst(RenderState::BLEND_ONE_MINUS_SRC_ALPHA);
            break;
        case BLEND_ADDITIVE:
            _batch->getStateBlock()->setBlend(true);
            _batch->getStateBlock()->setBlendSrc(RenderState::BLEND_SRC_ALPHA);
            _batch->getStateBlock()->setBlendDst(RenderState::BLEND_ONE);
            break;
        case BLEND_MULTIPLIED:
            _batch->getStateBlock()->setBlend(true);
            _batch->getStateBlock()->setBlendSrc(RenderState::BLEND_ZERO);
            _batch->getStateBlock()->setBlendDst(RenderState::BLEND_SRC_COLOR);
            break;
        default:
            GP_ERROR("Unsupported blend mode (%d).", mode);
            break;
    }
}

unsigned int Sprite::draw(bool wireframe)
{
    // Apply scene camera projection and translation offsets
    Vector3 position = Vector3::zero();
    if (_node && _node->getScene())
    {
        Camera* activeCamera = _node->getScene()->getActiveCamera();
        if (activeCamera)
        {
            Node* cameraNode = _node->getScene()->getActiveCamera()->getNode();
            if (cameraNode)
            {
                // Scene projection
                Matrix projectionMatrix;
                projectionMatrix = _node->getProjectionMatrix();
                _batch->setProjectionMatrix(projectionMatrix);

                // Camera translation offsets
                position.x -= cameraNode->getTranslationWorld().x;
                position.y -= cameraNode->getTranslationWorld().y;
            }
        }

        // Apply node translation offsets
        Vector3 translation = _node->getTranslationWorld();
        position.x += translation.x;
        position.y += translation.y;
        position.z += translation.z;
    }

    // Apply local offset translation offsets
    if ((_offset & OFFSET_HCENTER) == OFFSET_HCENTER) position.x -= _width * 0.5;
    if ((_offset & OFFSET_RIGHT) == OFFSET_RIGHT) position.x -= _width;
    if ((_offset & OFFSET_VCENTER) == OFFSET_VCENTER) position.y -= _height * 0.5f;
    if ((_offset & OFFSET_TOP) == OFFSET_TOP) position.y -= _height;
    if ((_offset & OFFSET_ANCHOR) == OFFSET_ANCHOR)
    {
        position.x -= _width * _anchor.x;
        position.y -= _height * _anchor.y;
    }

    // Apply node scale and rotation
    float rotationAngle = 0.0f;
    Vector2 scale = Vector2(_width, _height);
    if (_node)
    {
        // Apply node rotation
        const Quaternion& rot = _node->getRotation();
        if (rot.x != 0.0f || rot.y != 0.0f || rot.z != 0.0f)
            rotationAngle = rot.toAxisAngle(nullptr);

        // Apply node scale
        if (_node->getScaleX() != 1.0f) scale.x *= _node->getScaleX();
        if (_node->getScaleY() != 1.0f) scale.y *= _node->getScaleY();
    }

    // Apply flip flags
    if ((_flipFlags & FLIP_HORIZONTAL) == FLIP_HORIZONTAL)
    {
        position.x += scale.x;
        scale.x = -scale.x;
    }
    if ((_flipFlags & FLIP_VERTICAL) == FLIP_VERTICAL)
    {
        position.y += scale.y;
        scale.y = -scale.y;
    }

    // TODO: Proper batching from cache based on batching rules (image, layers, etc)
    _batch->start();
    _batch->draw(position,
                 _frames[_frameIndex],
                 scale,
                 Vector4(_color.x, _color.y, _color.z, _color.w * _opacity),
                 _anchor,
                 rotationAngle);
    _batch->finish();

    return 1;
}

Drawable* Sprite::clone(NodeCloneContext& context)
{
    Sprite* spriteClone = new Sprite();

    // Clone animations
    AnimationTarget::cloneInto(static_cast<AnimationTarget*>(spriteClone), context);

    // Get copied node if it exists
    if (Node* node = getNode())
    {
        Node* clonedNode = context.findClonedNode(node);
        if (clonedNode)
        {
            spriteClone->setNode(clonedNode);
        }
    }

    // Clone properties
    spriteClone->_width = _width;
    spriteClone->_height = _height;
    spriteClone->_offset = _offset;
    spriteClone->_anchor = _anchor;
    spriteClone->_flipFlags = _flipFlags;
    spriteClone->_opacity = _opacity;
    spriteClone->_color = _color;
    spriteClone->_blendMode = _blendMode;
    spriteClone->_frameCount = _frameCount;
    spriteClone->_frameStride = _frameStride;
    spriteClone->_framePadding = _framePadding;
    spriteClone->_frameIndex = _frameIndex;
    spriteClone->_batch = _batch;

    spriteClone->_frames = std::make_unique<Rectangle[]>(_frameCount);
    std::copy(_frames.get(), _frames.get() + _frameCount, spriteClone->_frames.get());

    return spriteClone;
}

int Sprite::getPropertyId(TargetType type, const std::string& propertyIdStr)
{
    if (type == AnimationTarget::TRANSFORM)
    {
        if (propertyIdStr == "ANIMATE_OPACITY")
        {
            return Sprite::ANIMATE_OPACITY;
        }
        else if (propertyIdStr == "ANIMATE_COLOR")
        {
            return Sprite::ANIMATE_COLOR;
        }
        else if (propertyIdStr == "ANIMATE_KEYFRAME")
        {
            return Sprite::ANIMATE_KEYFRAME;
        }
    }

    return AnimationTarget::getPropertyId(type, propertyIdStr);
}

unsigned int Sprite::getAnimationPropertyComponentCount(int propertyId) const
{
    switch (propertyId)
    {
        case ANIMATE_OPACITY:
            return 1;
        case ANIMATE_COLOR:
            return 4;
        case ANIMATE_KEYFRAME:
            return 1;
        default:
            return -1;
    }
}

void Sprite::getAnimationPropertyValue(int propertyId, AnimationValue* value)
{
    assert(value);

    switch (propertyId)
    {
        case ANIMATE_OPACITY:
            value->setFloat(0, _opacity);
            break;
        case ANIMATE_COLOR:
            value->setFloat(0, _color.x);
            value->setFloat(1, _color.y);
            value->setFloat(2, _color.z);
            value->setFloat(3, _color.w);
            break;
        case ANIMATE_KEYFRAME:
            value->setFloat(0, (float)_frameIndex);
            break;
        default:
            break;
    }
}

void Sprite::setAnimationPropertyValue(int propertyId, AnimationValue* value, float blendWeight)
{
    assert(value);

    switch (propertyId)
    {
        case ANIMATE_OPACITY:
            setOpacity(Curve::lerp(blendWeight, _opacity, value->getFloat(0)));
            break;
        case ANIMATE_COLOR:
            setColor(Vector4(Curve::lerp(blendWeight, _color.x, value->getFloat(0)),
                             Curve::lerp(blendWeight, _color.x, value->getFloat(1)),
                             Curve::lerp(blendWeight, _color.x, value->getFloat(2)),
                             Curve::lerp(blendWeight, _color.x, value->getFloat(3))));
            break;
        case ANIMATE_KEYFRAME:
            _frameIndex = (unsigned int)value->getFloat(0);
            break;
        default:
            break;
    }
}

} // namespace tractor

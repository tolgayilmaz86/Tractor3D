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

#include <ui/ThemeStyle.h>

namespace tractor
{

//----------------------------------------------------------------
/****************
 * Theme::Style *
 ****************/
Theme::Style::Style(Theme* theme,
                    const std::string& id,
                    float tw,
                    float th,
                    const Theme::Margin& margin,
                    const Theme::Padding& padding,
                    OverlayPtr normal,
                    OverlayPtr focus,
                    OverlayPtr active,
                    OverlayPtr disabled,
                    OverlayPtr hover)
    : _theme(theme), _id(id), _tw(tw), _th(th), _margin(margin), _padding(padding)
{
    _overlays[OVERLAY_NORMAL] = normal;
    _overlays[OVERLAY_FOCUS] = focus;
    _overlays[OVERLAY_ACTIVE] = active;
    _overlays[OVERLAY_DISABLED] = disabled;
    _overlays[OVERLAY_HOVER] = hover;
}

//----------------------------------------------------------------
Theme::Style::Style(const Style& copy)
{
    _theme = copy._theme;
    _id = copy._id;
    _margin = copy._margin;
    _padding = copy._padding;
    _tw = copy._tw;
    _th = copy._th;

    for (size_t i = 0; i < OVERLAY_MAX; i++)
    {
        if (copy._overlays[i])
            _overlays[i] = std::shared_ptr<Overlay>(new Theme::Style::Overlay(*copy._overlays[i]));
        else
            _overlays[i] = nullptr;
    }
}

//----------------------------------------------------------------
Theme::Style::~Style()
{
    // shared_ptr handles cleanup automatically
}

//----------------------------------------------------------------
/*************************
 * Theme::Style::Overlay *
 *************************/
Theme::Style::OverlayPtr Theme::Style::Overlay::create()
{
    return std::shared_ptr<Overlay>(new Overlay());
}

//----------------------------------------------------------------
Theme::Style::Overlay::Overlay(const Overlay& copy)
    : _skin(copy._skin), _cursor(copy._cursor), _imageList(copy._imageList), _font(copy._font)
{
    // Note: _skin, _cursor, _imageList are non-owning pointers to Theme-owned objects
    _fontSize = copy._fontSize;
    _alignment = copy._alignment;
    _textRightToLeft = copy._textRightToLeft;
    _textColor = Vector4(copy._textColor);
    _opacity = copy._opacity;
}

//----------------------------------------------------------------
Theme::Style::Overlay::~Overlay()
{
    // _skin, _cursor, _imageList are non-owning pointers - Theme owns them
    // _font is a shared_ptr and handles its own cleanup
}

//----------------------------------------------------------------
void Theme::Style::Overlay::setBorder(float top, float bottom, float left, float right)
{
    if (_skin)
    {
        _skin->_border.top = top;
        _skin->_border.bottom = bottom;
        _skin->_border.left = left;
        _skin->_border.right = right;
    }
}

//----------------------------------------------------------------
const Theme::Border& Theme::Style::Overlay::getBorder() const
{
    if (_skin)
    {
        return _skin->getBorder();
    }
    else
    {
        return Theme::Border::empty();
    }
}

//----------------------------------------------------------------
void Theme::Style::Overlay::setSkinColor(const Vector4& color)
{
    if (_skin)
    {
        _skin->_color.set(color);
    }
}

//----------------------------------------------------------------
const Vector4& Theme::Style::Overlay::getSkinColor() const noexcept
{
    if (_skin)
    {
        return _skin->getColor();
    }

    return Vector4::one();
}

//----------------------------------------------------------------
void Theme::Style::Overlay::setSkinRegion(const Rectangle& region, float tw, float th)
{
    assert(_skin);
    _skin->setRegion(region, tw, th);
}

//----------------------------------------------------------------
const Rectangle& Theme::Style::Overlay::getSkinRegion() const
{
    if (_skin)
    {
        return _skin->getRegion();
    }

    return Rectangle::empty();
}

//----------------------------------------------------------------
const Theme::UVs& Theme::Style::Overlay::getSkinUVs(Theme::Skin::SkinArea area) const
{
    if (_skin)
    {
        return _skin->_uvs[area];
    }

    return UVs::empty();
}

//----------------------------------------------------------------
void Theme::Style::Overlay::setFont(Font* font)
{
    if (_font.get() != font)
    {
        _font = font ? font->shared_from_this() : nullptr;
    }
}

//----------------------------------------------------------------
const Rectangle& Theme::Style::Overlay::getImageRegion(const std::string& id) const
{
    if (!_imageList)
    {
        return Rectangle::empty();
    }

    ThemeImage* image = _imageList->getImage(id);
    if (image)
    {
        return image->getRegion();
    }
    else
    {
        return Rectangle::empty();
    }
}

//----------------------------------------------------------------
void Theme::Style::Overlay::setImageRegion(const std::string& id,
                                           const Rectangle& region,
                                           float tw,
                                           float th)
{
    assert(_imageList);
    ThemeImage* image = _imageList->getImage(id);
    assert(image);
    image->_region.set(region);
    generateUVs(tw, th, region.x, region.y, region.width, region.height, &(image->_uvs));
}

//----------------------------------------------------------------
const Vector4& Theme::Style::Overlay::getImageColor(const std::string& id) const
{
    assert(_imageList);
    ThemeImage* image = _imageList->getImage(id);
    if (image)
    {
        return image->getColor();
    }
    else
    {
        return Vector4::zero();
    }
}

//----------------------------------------------------------------
void Theme::Style::Overlay::setImageColor(const std::string& id, const Vector4& color)
{
    assert(_imageList);
    ThemeImage* image = _imageList->getImage(id);
    assert(image);
    image->_color.set(color);
}

//----------------------------------------------------------------
const Theme::UVs& Theme::Style::Overlay::getImageUVs(const std::string& id) const
{
    assert(_imageList);
    ThemeImage* image = _imageList->getImage(id);
    if (image)
    {
        return image->getUVs();
    }
    else
    {
        return UVs::empty();
    }
}

//----------------------------------------------------------------
const Rectangle& Theme::Style::Overlay::getCursorRegion() const
{
    if (_cursor)
    {
        return _cursor->getRegion();
    }
    else
    {
        return Rectangle::empty();
    }
}

//----------------------------------------------------------------
void Theme::Style::Overlay::setCursorRegion(const Rectangle& region, float tw, float th)
{
    assert(_cursor);
    _cursor->_region.set(region);
    generateUVs(tw, th, region.x, region.y, region.width, region.height, &(_cursor->_uvs));
}

//----------------------------------------------------------------
const Vector4& Theme::Style::Overlay::getCursorColor() const
{
    if (_cursor)
    {
        return _cursor->getColor();
    }
    else
    {
        return Vector4::zero();
    }
}

//----------------------------------------------------------------
void Theme::Style::Overlay::setCursorColor(const Vector4& color)
{
    assert(_cursor);
    _cursor->_color.set(color);
}

//----------------------------------------------------------------
const Theme::UVs& Theme::Style::Overlay::getCursorUVs() const
{
    if (_cursor)
    {
        return _cursor->getUVs();
    }
    else
    {
        return UVs::empty();
    }
}

//----------------------------------------------------------------
void Theme::Style::Overlay::setSkin(Skin* skin)
{
    // Non-owning pointer - Theme owns the Skin
    _skin = skin;
}

//----------------------------------------------------------------
void Theme::Style::Overlay::setCursor(ThemeImage* cursor)
{
    // Non-owning pointer - Theme owns the ThemeImage
    _cursor = cursor;
}

//----------------------------------------------------------------
void Theme::Style::Overlay::setImageList(ImageList* imageList)
{
    // Non-owning pointer - Theme owns the ImageList
    _imageList = imageList;
}

//----------------------------------------------------------------
// Implementation of AnimationHandler
//----------------------------------------------------------------
unsigned int Theme::Style::Overlay::getAnimationPropertyComponentCount(int propertyId) const
{
    switch (propertyId)
    {
        case Theme::Style::Overlay::ANIMATE_OPACITY:
            return 1;
        default:
            return -1;
    }
}

//----------------------------------------------------------------
void Theme::Style::Overlay::getAnimationPropertyValue(int propertyId, AnimationValue* value)
{
    assert(value);

    switch (propertyId)
    {
        case ANIMATE_OPACITY:
            value->setFloat(0, _opacity);
            break;
        default:
            break;
    }
}

//----------------------------------------------------------------
void Theme::Style::Overlay::setAnimationPropertyValue(int propertyId,
                                                      AnimationValue* value,
                                                      float blendWeight)
{
    assert(value);

    switch (propertyId)
    {
        case ANIMATE_OPACITY:
        {
            _opacity = Curve::lerp(blendWeight, _opacity, value->getFloat(0));
            break;
        }
        default:
            break;
    }
}

} // namespace tractor
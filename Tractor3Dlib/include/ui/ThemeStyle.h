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

#include "pch.h"

#include "graphics/Rectangle.h"
#include "renderer/Font.h"
#include "renderer/Texture.h"
#include "scene/Properties.h"
#include "ui/Theme.h"
#include "utils/Ref.h"

namespace tractor
{

/**
 * Defines the style of a control.
 *
 * A style can have padding and margin values,
 * as well as overlays for each of the control's states.
 * Each overlay in turn can reference other theme classes to determine
 * the border, background, cursor, and image settings to use for
 * a particular state, as well as color and font settings, etcetera.
 */
class Theme::Style
{
    friend class Theme;
    friend class Control;
    friend class Container;
    friend class Form;

  public:
    /**
     * Get the theme this style belongs to.
     *
     * @return The theme this style belongs to.
     */
    Theme* getTheme() const noexcept { return _theme; }

  private:
    /**
     * A style has one overlay for each possible control state.
     */
    enum OverlayType
    {
        OVERLAY_NORMAL,
        OVERLAY_FOCUS,
        OVERLAY_ACTIVE,
        OVERLAY_DISABLED,
        OVERLAY_HOVER,
        OVERLAY_MAX
    };

    /**
     * This class represents a control's overlay for one of its states.
     */
    class Overlay : public Ref, public AnimationTarget
    {
        friend class Theme;
        friend class Theme::Style;
        friend class Control;
        friend class Container;
        friend class Form;

      private:
        static const int ANIMATE_OPACITY = 1;

        Overlay() = default;

        Overlay(const Overlay& copy);

        virtual ~Overlay();

        /**
         * Hidden copy assignment operator.
         */
        Overlay& operator=(const Overlay&) = delete;

        static Overlay* create();

        OverlayType getType() const noexcept { return OVERLAY_NORMAL; }

        float getOpacity() const noexcept { return _opacity; }

        void setOpacity(float opacity) noexcept { _opacity = opacity; }

        void setBorder(float top, float bottom, float left, float right);

        const Theme::Border& getBorder() const;

        void setSkinColor(const Vector4& color);

        const Vector4& getSkinColor() const noexcept;

        void setSkinRegion(const Rectangle& region, float tw, float th);

        const Rectangle& getSkinRegion() const;

        const Theme::UVs& getSkinUVs(Theme::Skin::SkinArea area) const;

        Font* getFont() const noexcept { return _font; }

        void setFont(Font* font);

        unsigned int getFontSize() const noexcept { return _fontSize; }

        void setFontSize(unsigned int fontSize) noexcept { _fontSize = fontSize; }

        Font::Justify getTextAlignment() const noexcept { return _alignment; }

        void setTextAlignment(Font::Justify alignment) { _alignment = alignment; }

        bool getTextRightToLeft() const noexcept { return _textRightToLeft; }

        void setTextRightToLeft(bool rightToLeft) noexcept { _textRightToLeft = rightToLeft; }

        const Vector4& getTextColor() const noexcept { return _textColor; }

        void setTextColor(const Vector4& color) noexcept { _textColor = color; }

        const Rectangle& getImageRegion(const std::string& id) const;

        void setImageRegion(const std::string& id, const Rectangle& region, float tw, float th);

        const Vector4& getImageColor(const std::string& id) const;

        void setImageColor(const std::string& id, const Vector4& color);

        const Theme::UVs& getImageUVs(const std::string& id) const;

        const Rectangle& getCursorRegion() const;

        void setCursorRegion(const Rectangle& region, float tw, float th);

        const Vector4& getCursorColor() const;

        void setCursorColor(const Vector4& color);

        const Theme::UVs& getCursorUVs() const;

        /**
         * @see AnimationTarget::getAnimationPropertyComponentCount
         */
        unsigned int getAnimationPropertyComponentCount(int propertyId) const;

        /**
         * @see AnimationTarget::getAnimationProperty
         */
        void getAnimationPropertyValue(int propertyId, AnimationValue* value);

        /**
         * @see AnimationTarget::setAnimationProperty
         */
        void setAnimationPropertyValue(int propertyId, AnimationValue* value, float blendWeight = 1.0f);

        void setSkin(Theme::Skin* Skin);

        Theme::Skin* getSkin() const noexcept { return _skin; }

        void setCursor(Theme::ThemeImage* cursor);

        Theme::ThemeImage* getCursor() const noexcept { return _cursor; }

        void setImageList(Theme::ImageList* imageList);

        Theme::ImageList* getImageList() const noexcept { return _imageList; }

        unsigned int _fontSize{ 0 };
        bool _textRightToLeft{ false };
        Font::Justify _alignment{ Font::ALIGN_TOP_LEFT };
        float _opacity{ 1.0f };
        Vector4 _textColor{ Vector4::one() };

        Skin* _skin{ nullptr };
        Theme::ThemeImage* _cursor{ nullptr };
        Theme::ImageList* _imageList{ nullptr };
        Font* _font{ nullptr };
    };

    /**
     * Constructor.
     */
    Style(Theme* theme,
          const std::string& id,
          float tw,
          float th,
          const Theme::Margin& margin,
          const Theme::Padding& padding,
          Overlay* normal,
          Overlay* focus,
          Overlay* active,
          Overlay* disabled,
          Overlay* hover);

    /**
     * Constructor.
     */
    Style(const Style& style);

    /**
     * Destructor.
     */
    ~Style();

    /**
     * Hidden copy assignment operator.
     */
    Style& operator=(const Style&) = delete;

    /**
     * Returns the Id of this Style.
     */
    const std::string& getId() const noexcept { return _id; }

    /**
     * Gets an overlay from the overlay type.
     */
    Theme::Style::Overlay* getOverlay(OverlayType overlayType) const noexcept
    {
        return _overlays[overlayType];
    }

    /**
     * Gets the Padding region of this style.
     */
    const Theme::Padding& getPadding() const noexcept { return _padding; }

    /**
     * Gets the Margin region of this style.
     */
    const Theme::Margin& getMargin() const noexcept { return _margin; }

    /**
     * Set this size of this Style's padding.
     *
     * Padding is the space between a Control's content (all icons and text) and its border.
     */
    void setPadding(float top, float bottom, float left, float right)
    {
        _padding.top = top;
        _padding.bottom = bottom;
        _padding.left = left;
        _padding.right = right;
    }

    /**
     * Set the size of this Style's margin.
     *
     * The margin is used by Layouts other than AbsoluteLayout to put space between Controls.
     */
    void setMargin(float top, float bottom, float left, float right)
    {
        _margin.top = top;
        _margin.bottom = bottom;
        _margin.left = left;
        _margin.right = right;
    }

    Theme* _theme{ nullptr };
    std::string _id{};
    float _tw{ 0 };
    float _th{ 0 };
    Theme::Margin _margin{};
    Theme::Padding _padding{};
    Overlay* _overlays[OVERLAY_MAX]{ nullptr };
};

} // namespace tractor

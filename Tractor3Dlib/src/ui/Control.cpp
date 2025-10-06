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

#include "ui/Control.h"

#include "framework/Game.h"
#include "ui/Form.h"
#include "ui/Theme.h"

namespace tractor
{

constexpr auto TYPE_NAME = "Control";

Control::Control()
{
    GP_REGISTER_SCRIPT_EVENTS();
}

Control::~Control()
{
    Form::verifyRemovedControlState(this);

    if (_listeners)
    {
        for (std::map<Control::Listener::EventType, std::list<Control::Listener*>*>::const_iterator itr =
                 _listeners->begin();
             itr != _listeners->end();
             ++itr)
        {
            std::list<Control::Listener*>* list = itr->second;
            SAFE_DELETE(list);
        }
        SAFE_DELETE(_listeners);
    }

    if (_style)
    {
        // Release the style's theme since we addRef'd it in initialize()
        _style->getTheme()->release();

        if (_styleOverridden)
        {
            SAFE_DELETE(_style);
        }
    }
}

Control::AutoSize Control::parseAutoSize(const std::string& str)
{
    if (str.empty()) return _autoSize;

    // Convert to uppercase for comparison
    std::string upperStr = str;
    std::transform(upperStr.begin(),
                   upperStr.end(),
                   upperStr.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    if (upperStr == "AUTO_SIZE_WIDTH") return AUTO_SIZE_WIDTH;
    if (upperStr == "AUTO_SIZE_HEIGHT") return AUTO_SIZE_HEIGHT;
    if (upperStr == "AUTO_SIZE_BOTH") return AUTO_SIZE_BOTH;

    return _autoSize;
}

void Control::initialize(const std::string& typeName, Theme::Style* style, Properties* properties)
{
    // Load our theme style
    if (properties)
    {
        // The style passed is in our parent control's style.
        // Attempt to load our own style from our parent style's theme.
        const auto& styleName = properties->getString("style", typeName);
        if (style)
        {
            // The passed in style is our parent control's style : attempt to load our style from it.
            _style = style->getTheme()->getStyle(styleName);
        }
        if (!_style)
        {
            // Use an empty style from our parent's theme
            _style = style->getTheme()->getEmptyStyle();
        }
    }
    else
    {
        // No properties passed in - the style passed in was explicity for us.
        _style = style;
    }

    if (!_style)
    {
        // Search for a style from the default theme that matches this control's name
        _style = Theme::getDefault()->getStyle(typeName);
        if (!_style)
        {
            // No style was found, use an empty style
            _style =
                style ? style->getTheme()->getEmptyStyle() : Theme::getDefault()->getEmptyStyle();
        }
    }
    // Increase the reference count of the style's theme while we hold the style
    _style->getTheme()->addRef();
    if (properties)
    {
        const auto& id = properties->getId();
        if (!id.empty()) _id = id;

        // Properties not defined by the style.
        const auto& alignmentString = properties->getString("alignment");
        _alignment = getAlignment(alignmentString);
        _consumeInputEvents = properties->getBool("consumeInputEvents", true);
        _visible = properties->getBool("visible", true);

        if (properties->exists("zIndex"))
        {
            _zIndex = properties->getInt("zIndex");
        }
        else
        {
            _zIndex = -1;
        }
        if (properties->exists("canFocus")) _canFocus = properties->getBool("canFocus", false);
        if (properties->exists("focusIndex"))
        {
            _focusIndex = properties->getInt("focusIndex");
        }
        else
        {
            _focusIndex = -1;
        }
        float bounds[2];
        bool boundsBits[2];
        auto position = properties->getString("position");
        if (!position.empty()
            && parseCoordPair(position, &bounds[0], &bounds[1], &boundsBits[0], &boundsBits[1]))
        {
            setX(bounds[0], boundsBits[0]);
            setY(bounds[1], boundsBits[1]);
        }
        else
        {
            if (properties->exists("x"))
            {
                bounds[0] = parseCoord(properties->getString("x", "0"), &boundsBits[0]);
                setX(bounds[0], boundsBits[0]);
            }
            if (properties->exists("y"))
            {
                bounds[1] = parseCoord(properties->getString("y", "0"), &boundsBits[1]);
                setY(bounds[1], boundsBits[1]);
            }
        }
        // If there is an explicitly specified size, width or height, unset the corresponding autoSize bit
        auto size = properties->getString("size");
        if (!size.empty()
            && parseCoordPair(size, &bounds[0], &bounds[1], &boundsBits[0], &boundsBits[1]))
        {
            setWidth(bounds[0], boundsBits[0]);
            setHeight(bounds[1], boundsBits[1]);
        }
        else
        {
            auto width = properties->getString("width");
            if (!width.empty())
            {
                bounds[0] = parseCoord(width, &boundsBits[0]);
                setWidth(bounds[0], boundsBits[0]);
            }
            auto height = properties->getString("height");
            if (!height.empty())
            {
                bounds[1] = parseCoord(height, &boundsBits[1]);
                setHeight(bounds[1], boundsBits[1]);
            }
        }
        // Backwards Compatibility: Support deprecated autoWidth and autoHeight properties,
        // which resolve to width=100% and height=100%.
        if (properties->getBool("autoWidth")) setWidth(1.0f, true);
        if (properties->getBool("autoHeight")) setHeight(1.0f, true);

        // Parse the auto-size mode for the control (this overrides explicit sizes and legacy autoWidth/autoHeight)
        _autoSize = parseAutoSize(properties->getString("autoSize"));

        // If there is are simple padding or margin variables, parse them
        if (properties->exists("padding"))
        {
            float pad = properties->getFloat("padding");
            setPadding(pad, pad, pad, pad);
        }
        if (properties->exists("margin"))
        {
            float margin = properties->getFloat("margin");
            setPadding(margin, margin, margin, margin);
        }

        if (properties->exists("enabled"))
        {
            setEnabled(properties->getBool("enabled"));
        }

        // Register script listeners for control events
        if (properties->exists("script")) addScript(properties->getString("script"));

        // Potentially override themed properties for all states.
        overrideThemedProperties(properties, STATE_ALL);

        // Override themed properties on specific states.
        Properties* innerSpace = properties->getNextNamespace();
        while (innerSpace != nullptr)
        {
            std::string spaceName(innerSpace->getNamespace());
            std::transform(spaceName.begin(), spaceName.end(), spaceName.begin(), (int (*)(int))toupper);
            if (spaceName == "STATENORMAL")
            {
                overrideThemedProperties(innerSpace, NORMAL);
            }
            else if (spaceName == "STATEFOCUS")
            {
                overrideThemedProperties(innerSpace, FOCUS);
            }
            else if (spaceName == "STATEACTIVE")
            {
                overrideThemedProperties(innerSpace, ACTIVE);
            }
            else if (spaceName == "STATEDISABLED")
            {
                overrideThemedProperties(innerSpace, DISABLED);
            }
            else if (spaceName == "STATEHOVER")
            {
                overrideThemedProperties(innerSpace, HOVER);
            }
            else if (spaceName == "MARGIN")
            {
                setMargin(innerSpace->getFloat("top"),
                          innerSpace->getFloat("bottom"),
                          innerSpace->getFloat("left"),
                          innerSpace->getFloat("right"));
            }
            else if (spaceName == "PADDING")
            {
                setPadding(innerSpace->getFloat("top"),
                           innerSpace->getFloat("bottom"),
                           innerSpace->getFloat("left"),
                           innerSpace->getFloat("right"));
            }
            innerSpace = properties->getNextNamespace();
        }
    }
}

const std::string& Control::getTypeName() const noexcept { return TYPE_NAME; }

void Control::setX(float x, bool percentage) noexcept
{
    if (_relativeBounds.x != x || percentage != ((_boundsBits & BOUNDS_X_PERCENTAGE_BIT) != 0))
    {
        setXInternal(x, percentage);
        setDirty(DIRTY_BOUNDS);
    }
}

void Control::setXInternal(float x, bool percentage) noexcept
{
    _relativeBounds.x = x;
    if (percentage)
    {
        _boundsBits |= BOUNDS_X_PERCENTAGE_BIT;
    }
    else
    {
        _boundsBits &= ~BOUNDS_X_PERCENTAGE_BIT;
        _bounds.x = x;
    }
}

bool Control::isXPercentage() const noexcept
{
    return (_boundsBits & BOUNDS_X_PERCENTAGE_BIT) != 0;
}

void Control::setY(float y, bool percentage) noexcept
{
    if (_relativeBounds.y != y || percentage != ((_boundsBits & BOUNDS_Y_PERCENTAGE_BIT) != 0))
    {
        setYInternal(y, percentage);
        setDirty(DIRTY_BOUNDS);
    }
}

void Control::setYInternal(float y, bool percentage) noexcept
{
    _relativeBounds.y = y;
    if (percentage)
    {
        _boundsBits |= BOUNDS_Y_PERCENTAGE_BIT;
    }
    else
    {
        _boundsBits &= ~BOUNDS_Y_PERCENTAGE_BIT;
        _bounds.y = y;
    }
}

bool Control::isYPercentage() const noexcept
{
    return (_boundsBits & BOUNDS_Y_PERCENTAGE_BIT) != 0;
}

void Control::setWidth(float width, bool percentage)
{
    _autoSize = (AutoSize)(_autoSize & ~AUTO_SIZE_WIDTH);

    if (_relativeBounds.width != width
        || percentage != ((_boundsBits & BOUNDS_WIDTH_PERCENTAGE_BIT) != 0))
    {
        setWidthInternal(width, percentage);
        setDirty(DIRTY_BOUNDS);
    }
}

void Control::setWidthInternal(float width, bool percentage) noexcept
{
    _relativeBounds.width = width;
    if (percentage)
    {
        _boundsBits |= BOUNDS_WIDTH_PERCENTAGE_BIT;
    }
    else
    {
        _boundsBits &= ~BOUNDS_WIDTH_PERCENTAGE_BIT;
        _bounds.width = width;
    }
}

bool Control::isWidthPercentage() const noexcept
{
    return (_boundsBits & BOUNDS_WIDTH_PERCENTAGE_BIT) != 0;
}

void Control::setHeight(float height, bool percentage) noexcept
{
    _autoSize = (AutoSize)(_autoSize & ~AUTO_SIZE_HEIGHT);

    if (_relativeBounds.height != height
        || percentage != ((_boundsBits & BOUNDS_HEIGHT_PERCENTAGE_BIT) != 0))
    {
        setHeightInternal(height, percentage);
        setDirty(DIRTY_BOUNDS);
    }
}

void Control::setHeightInternal(float height, bool percentage) noexcept
{
    _relativeBounds.height = height;
    if (percentage)
    {
        _boundsBits |= BOUNDS_HEIGHT_PERCENTAGE_BIT;
    }
    else
    {
        _boundsBits &= ~BOUNDS_HEIGHT_PERCENTAGE_BIT;
        _bounds.height = height;
    }
}

bool Control::isHeightPercentage() const noexcept
{
    return (_boundsBits & BOUNDS_HEIGHT_PERCENTAGE_BIT) != 0;
}

void Control::setPosition(float x, float y) noexcept
{
    setX(x);
    setY(y);
}

void Control::setSize(float width, float height) noexcept
{
    setWidth(width);
    setHeight(height);
}

void Control::setBounds(const Rectangle& bounds) noexcept
{
    setX(bounds.x);
    setY(bounds.y);
    setWidth(bounds.width);
    setHeight(bounds.height);
}

void Control::setAlignment(Alignment alignment) noexcept
{
    if (_alignment != alignment)
    {
        _alignment = alignment;
        setDirty(DIRTY_BOUNDS);
    }
}

void Control::setAutoSize(AutoSize mode) noexcept
{
    if (_autoSize != mode)
    {
        _autoSize = mode;
        setDirty(DIRTY_BOUNDS);
    }
}

void Control::setVisible(bool visible)
{
    if (_visible != visible)
    {
        _visible = visible;

        if (!_visible) Form::controlDisabled(this);

        setDirty(DIRTY_BOUNDS);

        // force to update parent boundaries when child is hidden
        Control* parent = _parent;
        while (parent
               && (parent->_autoSize != AUTO_SIZE_NONE
                   || static_cast<Container*>(parent)->getLayout()->getType()
                          != Layout::LAYOUT_ABSOLUTE))
        {
            parent->setDirty(DIRTY_BOUNDS);
            parent = parent->_parent;
        }
    }
}

bool Control::isVisibleInHierarchy() const
{
    if (!_visible) return false;

    if (_parent) return _parent->isVisibleInHierarchy();

    return true;
}

bool Control::hasFocus() const noexcept
{
    return (Form::getFocusControl() == this);
}

bool Control::setFocus()
{
    if (Form::getFocusControl() != this && canFocus())
    {
        Form::setFocusControl(this);
        return true;
    }

    return false;
}

void Control::setOpacity(float opacity, unsigned char states)
{
    overrideStyle();
    Theme::Style::Overlay* overlays[Theme::Style::OVERLAY_MAX] = { 0 };
    getOverlays(states, overlays);

    for (size_t i = 0; i < Theme::Style::OVERLAY_MAX; ++i)
    {
        if (overlays[i]) overlays[i]->setOpacity(opacity);
    }
}

float Control::getOpacity(State state) const
{
    Theme::Style::Overlay* overlay = getOverlay(state);
    assert(overlay);
    return overlay->getOpacity();
}

void Control::setEnabled(bool enabled)
{
    if (enabled != isEnabled())
    {
        if (!enabled)
        {
            Form::controlDisabled(this);
        }
        _state = enabled ? NORMAL : DISABLED;
        setDirty(DIRTY_STATE);
    }
}

bool Control::isEnabledInHierarchy() const
{
    if (!isEnabled()) return false;
    if (_parent) return _parent->isEnabledInHierarchy();

    return true;
}

void Control::setBorder(float top, float bottom, float left, float right, unsigned char states)
{
    overrideStyle();
    Theme::Style::Overlay* overlays[Theme::Style::OVERLAY_MAX] = { 0 };
    getOverlays(states, overlays);

    for (size_t i = 0; i < Theme::Style::OVERLAY_MAX; ++i)
    {
        if (overlays[i]) overlays[i]->setBorder(top, bottom, left, right);
    }
    setDirty(DIRTY_BOUNDS);
}

const Theme::Border& Control::getBorder(State state) const
{
    Theme::Style::Overlay* overlay = getOverlay(state);
    assert(overlay);
    return overlay->getBorder();
}

void Control::setSkinRegion(const Rectangle& region, unsigned char states)
{
    overrideStyle();
    Theme::Style::Overlay* overlays[Theme::Style::OVERLAY_MAX] = { 0 };
    getOverlays(states, overlays);

    for (size_t i = 0; i < Theme::Style::OVERLAY_MAX; ++i)
    {
        if (overlays[i]) overlays[i]->setSkinRegion(region, _style->_tw, _style->_th);
    }
}

const Rectangle& Control::getSkinRegion(State state) const
{
    Theme::Style::Overlay* overlay = getOverlay(state);
    assert(overlay);
    return overlay->getSkinRegion();
}

void Control::setSkinColor(const Vector4& color, unsigned char states)
{
    overrideStyle();
    Theme::Style::Overlay* overlays[Theme::Style::OVERLAY_MAX] = { 0 };
    getOverlays(states, overlays);

    for (size_t i = 0; i < Theme::Style::OVERLAY_MAX; ++i)
    {
        if (overlays[i]) overlays[i]->setSkinColor(color);
    }
}

const Vector4& Control::getSkinColor(State state) const
{
    Theme::Style::Overlay* overlay = getOverlay(state);
    assert(overlay);
    return overlay->getSkinColor();
}

void Control::setMargin(float top, float bottom, float left, float right)
{
    assert(_style);
    overrideStyle();
    _style->setMargin(top, bottom, left, right);
    setDirty(DIRTY_BOUNDS);
}

const Theme::Margin& Control::getMargin() const
{
    assert(_style);
    return _style->getMargin();
}

void Control::setPadding(float top, float bottom, float left, float right)
{
    assert(_style);
    overrideStyle();
    _style->setPadding(top, bottom, left, right);
    setDirty(DIRTY_BOUNDS);
}

const Theme::Padding& Control::getPadding() const
{
    assert(_style);
    return _style->getPadding();
}

void Control::setImageRegion(const std::string& id, const Rectangle& region, unsigned char states)
{
    overrideStyle();
    Theme::Style::Overlay* overlays[Theme::Style::OVERLAY_MAX] = { 0 };
    getOverlays(states, overlays);

    for (size_t i = 0; i < Theme::Style::OVERLAY_MAX; ++i)
    {
        if (overlays[i]) overlays[i]->setImageRegion(id, region, _style->_tw, _style->_th);
    }
}

const Rectangle& Control::getImageRegion(const std::string& id, State state) const
{
    Theme::Style::Overlay* overlay = getOverlay(state);
    assert(overlay);
    return overlay->getImageRegion(id);
}

void Control::setImageColor(const std::string& id, const Vector4& color, unsigned char states)
{
    overrideStyle();
    Theme::Style::Overlay* overlays[Theme::Style::OVERLAY_MAX] = { 0 };
    getOverlays(states, overlays);

    for (size_t i = 0; i < Theme::Style::OVERLAY_MAX; ++i)
    {
        if (overlays[i]) overlays[i]->setImageColor(id, color);
    }
}

const Vector4& Control::getImageColor(const std::string& id, State state) const
{
    Theme::Style::Overlay* overlay = getOverlay(state);
    assert(overlay);
    return overlay->getImageColor(id);
}

const Theme::UVs& Control::getImageUVs(const std::string& id, State state) const
{
    Theme::Style::Overlay* overlay = getOverlay(state);
    assert(overlay);
    return overlay->getImageUVs(id);
}

void Control::setCursorRegion(const Rectangle& region, unsigned char states)
{
    overrideStyle();
    Theme::Style::Overlay* overlays[Theme::Style::OVERLAY_MAX] = { 0 };
    getOverlays(states, overlays);

    for (size_t i = 0; i < Theme::Style::OVERLAY_MAX; ++i)
    {
        if (overlays[i]) overlays[i]->setCursorRegion(region, _style->_tw, _style->_th);
    }
}

const Rectangle& Control::getCursorRegion(State state) const
{
    Theme::Style::Overlay* overlay = getOverlay(state);
    assert(overlay);
    return overlay->getCursorRegion();
}

void Control::setCursorColor(const Vector4& color, unsigned char states)
{
    overrideStyle();
    Theme::Style::Overlay* overlays[Theme::Style::OVERLAY_MAX] = { 0 };
    getOverlays(states, overlays);

    for (size_t i = 0; i < Theme::Style::OVERLAY_MAX; ++i)
    {
        if (overlays[i]) overlays[i]->setCursorColor(color);
    }
}

const Vector4& Control::getCursorColor(State state)
{
    Theme::Style::Overlay* overlay = getOverlay(state);
    assert(overlay);
    return overlay->getCursorColor();
}

const Theme::UVs& Control::getCursorUVs(State state)
{
    Theme::Style::Overlay* overlay = getOverlay(state);
    assert(overlay);
    return overlay->getCursorUVs();
}

void Control::setFont(Font* font, unsigned char states)
{
    overrideStyle();
    Theme::Style::Overlay* overlays[Theme::Style::OVERLAY_MAX] = { 0 };
    getOverlays(states, overlays);

    for (size_t i = 0; i < Theme::Style::OVERLAY_MAX; ++i)
    {
        if (overlays[i]) overlays[i]->setFont(font);
    }

    if (_autoSize != AUTO_SIZE_NONE) setDirty(DIRTY_BOUNDS);
}

Font* Control::getFont(State state) const
{
    Theme::Style::Overlay* overlay = getOverlay(state);
    assert(overlay);
    return overlay->getFont();
}

void Control::setFontSize(unsigned int fontSize, unsigned char states)
{
    overrideStyle();
    Theme::Style::Overlay* overlays[Theme::Style::OVERLAY_MAX] = { 0 };
    getOverlays(states, overlays);

    for (size_t i = 0; i < Theme::Style::OVERLAY_MAX; ++i)
    {
        if (overlays[i]) overlays[i]->setFontSize(fontSize);
    }

    if (_autoSize != AUTO_SIZE_NONE) setDirty(DIRTY_BOUNDS);
}

unsigned int Control::getFontSize(State state) const
{
    Theme::Style::Overlay* overlay = getOverlay(state);
    assert(overlay);
    return overlay->getFontSize();
}

void Control::setTextColor(const Vector4& color, unsigned char states)
{
    overrideStyle();
    Theme::Style::Overlay* overlays[Theme::Style::OVERLAY_MAX] = { 0 };
    getOverlays(states, overlays);

    for (size_t i = 0; i < Theme::Style::OVERLAY_MAX; ++i)
    {
        if (overlays[i]) overlays[i]->setTextColor(color);
    }
}

const Vector4& Control::getTextColor(State state) const
{
    Theme::Style::Overlay* overlay = getOverlay(state);
    assert(overlay);
    return overlay->getTextColor();
}

void Control::setTextAlignment(Font::Justify alignment, unsigned char states)
{
    overrideStyle();
    Theme::Style::Overlay* overlays[Theme::Style::OVERLAY_MAX] = { 0 };
    getOverlays(states, overlays);

    for (size_t i = 0; i < Theme::Style::OVERLAY_MAX; ++i)
    {
        if (overlays[i]) overlays[i]->setTextAlignment(alignment);
    }
}

Font::Justify Control::getTextAlignment(State state) const
{
    Theme::Style::Overlay* overlay = getOverlay(state);
    assert(overlay);
    return overlay->getTextAlignment();
}

void Control::setTextRightToLeft(bool rightToLeft, unsigned char states)
{
    overrideStyle();
    Theme::Style::Overlay* overlays[Theme::Style::OVERLAY_MAX] = { 0 };
    getOverlays(states, overlays);

    for (size_t i = 0; i < Theme::Style::OVERLAY_MAX; ++i)
    {
        if (overlays[i]) overlays[i]->setTextRightToLeft(rightToLeft);
    }
}

bool Control::getTextRightToLeft(State state) const
{
    Theme::Style::Overlay* overlay = getOverlay(state);
    assert(overlay);
    return overlay->getTextRightToLeft();
}

void Control::setStyle(Theme::Style* style)
{
    if (style != _style)
    {
        _style = style;
        setDirty(DIRTY_BOUNDS);
    }
}

Control::State Control::getState() const noexcept
{
    if (Form::getFocusControl() == this)
    {
        // Active is the only state that overrides focus state
        return _state == ACTIVE ? ACTIVE : FOCUS;
    }

    return _state;
}

Theme::Style::OverlayType Control::getOverlayType() const noexcept
{
    switch (getState())
    {
        case Control::NORMAL:
            return Theme::Style::OVERLAY_NORMAL;
        case Control::FOCUS:
            return Theme::Style::OVERLAY_FOCUS;
        case Control::ACTIVE:
            return Theme::Style::OVERLAY_ACTIVE;
        case Control::DISABLED:
            return Theme::Style::OVERLAY_DISABLED;
        case Control::HOVER:
            return Theme::Style::OVERLAY_HOVER;
        default:
            return Theme::Style::OVERLAY_NORMAL;
    }
}

void Control::setZIndex(int zIndex)
{
    if (zIndex != _zIndex)
    {
        _zIndex = zIndex;

        if (_parent)
        {
            _parent->sortControls();
        }
    }
}

void Control::addListener(Control::Listener* listener, int eventFlags)
{
    assert(listener);

    if ((eventFlags & Control::Listener::PRESS) == Control::Listener::PRESS)
    {
        addSpecificListener(listener, Control::Listener::PRESS);
    }

    if ((eventFlags & Control::Listener::RELEASE) == Control::Listener::RELEASE)
    {
        addSpecificListener(listener, Control::Listener::RELEASE);
    }

    if ((eventFlags & Control::Listener::CLICK) == Control::Listener::CLICK)
    {
        addSpecificListener(listener, Control::Listener::CLICK);
    }

    if ((eventFlags & Control::Listener::VALUE_CHANGED) == Control::Listener::VALUE_CHANGED)
    {
        addSpecificListener(listener, Control::Listener::VALUE_CHANGED);
    }

    if ((eventFlags & Control::Listener::TEXT_CHANGED) == Control::Listener::TEXT_CHANGED)
    {
        addSpecificListener(listener, Control::Listener::TEXT_CHANGED);
    }

    if ((eventFlags & Control::Listener::ACTIVATED) == Control::Listener::ACTIVATED)
    {
        addSpecificListener(listener, Control::Listener::ACTIVATED);
    }
}

void Control::removeListener(Control::Listener* listener)
{
    if (_listeners == nullptr || listener == nullptr) return;

    for (std::map<Control::Listener::EventType, std::list<Control::Listener*>*>::iterator itr =
             _listeners->begin();
         itr != _listeners->end();)
    {
        itr->second->remove(listener);

        if (itr->second->empty())
        {
            std::list<Control::Listener*>* list = itr->second;
            _listeners->erase(itr++);
            SAFE_DELETE(list);
        }
        else
            ++itr;
    }

    if (_listeners->empty()) SAFE_DELETE(_listeners);
}

void Control::addSpecificListener(Control::Listener* listener, Control::Listener::EventType eventType)
{
    assert(listener);

    if (!_listeners)
    {
        _listeners = new std::map<Control::Listener::EventType, std::list<Control::Listener*>*>();
    }

    std::map<Control::Listener::EventType, std::list<Control::Listener*>*>::const_iterator itr =
        _listeners->find(eventType);
    if (itr == _listeners->end())
    {
        _listeners->insert(std::make_pair(eventType, new std::list<Control::Listener*>()));
        itr = _listeners->find(eventType);
    }

    std::list<Control::Listener*>* listenerList = itr->second;
    listenerList->push_back(listener);
}

bool Control::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex)
{
    return false;
}

bool Control::keyEvent(Keyboard::KeyEvent evt, int key) { return false; }

bool Control::mouseEvent(Mouse::MouseEvent evt, int x, int y, int wheelDelta)
{
    // Return false instead of _consumeInputEvents to allow handling to be
    // routed to touchEvent before consuming.
    return false;
}

void Control::notifyListeners(Control::Listener::EventType eventType)
{
    // This method runs untrusted code by notifying listeners of events.
    // If the user calls exit() or otherwise releases this control, we
    // need to keep it alive until the method returns.
    this->addRef();

    controlEvent(eventType);

    if (_listeners)
    {
        if (auto itr = _listeners->find(eventType); itr != _listeners->end())
        {
            auto* listenerList = itr->second;

            // Iterate over the listeners in the list and call controlEvent on each
            for (auto* listener : *listenerList)
            {
                assert(listener);
                listener->controlEvent(this, eventType);
            }
        }
    }

    fireScriptEvent<void>(GP_GET_SCRIPT_EVENT(Control, controlEvent),
                          dynamic_cast<void*>(this),
                          eventType);

    this->release();
}

void Control::update(float elapsedTime)
{
    State state = getState();

    // Update visual state if it's dirty
    if (_dirtyBits & DIRTY_STATE) updateState(getState());

    // Since opacity is pre-multiplied, we compute it every frame so that we don't need to
    // dirty the entire hierarchy any time a state changes (which could affect opacity).
    _opacity = getOpacity(state);
    if (_parent) _opacity *= _parent->_opacity;
}

void Control::updateState(State state)
{
    // Clear dirty state bit
    _dirtyBits &= ~DIRTY_STATE;

    // Cache themed attributes for performance.
    _skin = getSkin(state);
}

bool Control::updateBoundsInternal(const Vector2& offset)
{
    // If our state is currently dirty, update it here so that any rendering state objects needed
    // for bounds computation are accessible.
    State state = getState();
    if (_dirtyBits & DIRTY_STATE)
    {
        updateState(state);
        _dirtyBits &= ~DIRTY_STATE;
    }

    // If we are a container, always update child bounds first
    bool changed = false;
    if (isContainer()) changed = static_cast<Container*>(this)->updateChildBounds();

    // Clear our dirty bounds bit
    bool dirtyBounds = (_dirtyBits & DIRTY_BOUNDS) != 0;
    _dirtyBits &= ~DIRTY_BOUNDS;

    if (dirtyBounds)
    {
        // Store old bounds so we can determine if they change
        Rectangle oldAbsoluteBounds(_absoluteBounds);
        Rectangle oldAbsoluteClipBounds(_absoluteClipBounds);
        Rectangle oldViewportBounds(_viewportBounds);
        Rectangle oldViewportClipBounds(_viewportClipBounds);

        updateBounds();
        updateAbsoluteBounds(offset);

        if (_absoluteBounds != oldAbsoluteBounds || _absoluteClipBounds != oldAbsoluteClipBounds
            || _viewportBounds != oldViewportBounds || _viewportClipBounds != oldViewportClipBounds)
        {
            if (isContainer()) static_cast<Container*>(this)->setChildrenDirty(DIRTY_BOUNDS, true);
            changed = true;
        }
    }

    return changed;
}

void Control::updateBounds()
{
    Game* game = Game::getInstance();

    const Rectangle parentAbsoluteBounds =
        _parent ? _parent->_viewportBounds
                : Rectangle(0, 0, game->getViewport().width, game->getViewport().height);

    const Theme::Margin& margin = _style->getMargin();

    // Calculate local unclipped bounds.
    _bounds.set(_relativeBounds);
    if (isXPercentage()) _bounds.x = _bounds.x * parentAbsoluteBounds.width + margin.left;
    if (isYPercentage()) _bounds.y = _bounds.y * parentAbsoluteBounds.height + margin.top;
    if (isWidthPercentage()) _bounds.width *= parentAbsoluteBounds.width;
    if (isHeightPercentage()) _bounds.height *= parentAbsoluteBounds.height;

    // Apply control alignment
    if (_alignment != Control::ALIGN_TOP_LEFT)
    {
        const Theme::Margin& margin = _style->getMargin();
        const Rectangle& parentBounds =
            _parent ? _parent->getBounds()
                    : Rectangle(0, 0, game->getViewport().width, game->getViewport().height);
        const Theme::Border& parentBorder =
            _parent ? _parent->getBorder(_parent->getState()) : Theme::Border::empty();
        const Theme::Padding& parentPadding =
            _parent ? _parent->getPadding() : Theme::Padding::empty();

        float clipWidth, clipHeight;
        if (_parent && (_parent->getScroll() != Container::SCROLL_NONE))
        {
            const Rectangle& verticalScrollBarBounds =
                _parent->getImageRegion("verticalScrollBar", _parent->getState());
            const Rectangle& horizontalScrollBarBounds =
                _parent->getImageRegion("horizontalScrollBar", _parent->getState());
            clipWidth = parentBounds.width - parentBorder.left - parentBorder.right
                        - parentPadding.left - parentPadding.right - verticalScrollBarBounds.width;
            clipHeight = parentBounds.height - parentBorder.top - parentBorder.bottom
                         - parentPadding.top - parentPadding.bottom
                         - horizontalScrollBarBounds.height;
        }
        else
        {
            clipWidth = parentBounds.width - parentBorder.left - parentBorder.right
                        - parentPadding.left - parentPadding.right;
            clipHeight = parentBounds.height - parentBorder.top - parentBorder.bottom
                         - parentPadding.top - parentPadding.bottom;
        }

        // Vertical alignment
        if ((_alignment & Control::ALIGN_BOTTOM) == Control::ALIGN_BOTTOM)
        {
            _bounds.y = clipHeight - _bounds.height - margin.bottom;
        }
        else if ((_alignment & Control::ALIGN_VCENTER) == Control::ALIGN_VCENTER)
        {
            _bounds.y =
                clipHeight * 0.5f - _bounds.height * 0.5f + (margin.top - margin.bottom) * 0.5f;
        }
        else if ((_alignment & Control::ALIGN_TOP) == Control::ALIGN_TOP)
        {
            _bounds.y = margin.top;
        }

        // Horizontal alignment
        if ((_alignment & Control::ALIGN_RIGHT) == Control::ALIGN_RIGHT)
        {
            _bounds.x = clipWidth - _bounds.width - margin.right;
        }
        else if ((_alignment & Control::ALIGN_HCENTER) == Control::ALIGN_HCENTER)
        {
            _bounds.x = clipWidth * 0.5f - _bounds.width * 0.5f + (margin.left - margin.right) * 0.5f;
        }
        else if ((_alignment & Control::ALIGN_LEFT) == Control::ALIGN_LEFT)
        {
            _bounds.x = margin.left;
        }
    }
}

void Control::updateAbsoluteBounds(const Vector2& offset)
{
    Game* game = Game::getInstance();

    const Rectangle parentAbsoluteBounds =
        _parent ? _parent->_viewportBounds
                : Rectangle(0, 0, game->getViewport().width, game->getViewport().height);
    const Rectangle parentAbsoluteClip =
        _parent ? _parent->_viewportClipBounds : parentAbsoluteBounds;

    // Compute content area padding values
    const Theme::Border& border = getBorder(NORMAL);
    const Theme::Padding& padding = getPadding();
    float lpadding = border.left + padding.left;
    float rpadding = border.right + padding.right;
    float tpadding = border.top + padding.top;
    float bpadding = border.bottom + padding.bottom;
    float hpadding = lpadding + rpadding;
    float vpadding = tpadding + bpadding;

    // Calculate absolute unclipped bounds
    _absoluteBounds.set(parentAbsoluteBounds.x + offset.x + _bounds.x,
                        parentAbsoluteBounds.y + offset.y + _bounds.y,
                        _bounds.width,
                        _bounds.height);

    // Calculate absolute clipped bounds
    Rectangle::intersect(_absoluteBounds, parentAbsoluteClip, &_absoluteClipBounds);

    // Calculate the local clipped bounds
    _clipBounds.set(max(_absoluteClipBounds.x - _absoluteBounds.x, 0.0f),
                    max(_absoluteClipBounds.y - _absoluteBounds.y, 0.0f),
                    _absoluteClipBounds.width,
                    _absoluteClipBounds.height);

    // Calculate the absolute unclipped viewport bounds (content area, which does not include border and padding)
    _viewportBounds.set(_absoluteBounds.x + lpadding,
                        _absoluteBounds.y + tpadding,
                        _absoluteBounds.width - hpadding,
                        _absoluteBounds.height - vpadding);

    // Calculate the absolute clipped viewport bounds
    Rectangle::intersect(_viewportBounds, parentAbsoluteClip, &_viewportClipBounds);
}

void Control::startBatch(Form* form, SpriteBatch* batch) { form->startBatch(batch); }

void Control::finishBatch(Form* form, SpriteBatch* batch) { form->finishBatch(batch); }

unsigned int Control::draw(Form* form, const Rectangle& clip)
{
    if (!_visible) return 0;

    unsigned int drawCalls = drawBorder(form, clip);
    drawCalls += drawImages(form, clip);
    drawCalls += drawText(form, clip);
    return drawCalls;
}

unsigned int Control::drawBorder(Form* form, const Rectangle& clip)
{
    if (!form || !_skin || _absoluteBounds.width <= 0 || _absoluteBounds.height <= 0) return 0;

    unsigned int drawCalls = 0;

    SpriteBatch* batch = _style->getTheme()->getSpriteBatch();
    startBatch(form, batch);

    // Get the border and background images for this control's current state.
    const Theme::UVs& topLeft = _skin->getUVs(Theme::Skin::TOP_LEFT);
    const Theme::UVs& top = _skin->getUVs(Theme::Skin::TOP);
    const Theme::UVs& topRight = _skin->getUVs(Theme::Skin::TOP_RIGHT);
    const Theme::UVs& left = _skin->getUVs(Theme::Skin::LEFT);
    const Theme::UVs& center = _skin->getUVs(Theme::Skin::CENTER);
    const Theme::UVs& right = _skin->getUVs(Theme::Skin::RIGHT);
    const Theme::UVs& bottomLeft = _skin->getUVs(Theme::Skin::BOTTOM_LEFT);
    const Theme::UVs& bottom = _skin->getUVs(Theme::Skin::BOTTOM);
    const Theme::UVs& bottomRight = _skin->getUVs(Theme::Skin::BOTTOM_RIGHT);

    // Calculate screen-space positions.
    const Theme::Border& border = getBorder(getState());
    const Theme::Padding& padding = getPadding();
    Vector4 skinColor = _skin->getColor();
    skinColor.w *= _opacity;

    float midWidth = _absoluteBounds.width - border.left - border.right;
    float midHeight = _absoluteBounds.height - border.top - border.bottom;
    float midX = _absoluteBounds.x + border.left;
    float midY = _absoluteBounds.y + border.top;
    float rightX = _absoluteBounds.x + _absoluteBounds.width - border.right;
    float bottomY = _absoluteBounds.y + _absoluteBounds.height - border.bottom;

    // Draw themed border sprites.
    if (!border.left && !border.right && !border.top && !border.bottom)
    {
        // No border, just draw the image.
        batch->draw(_absoluteBounds.x,
                    _absoluteBounds.y,
                    _absoluteBounds.width,
                    _absoluteBounds.height,
                    center.u1,
                    center.v1,
                    center.u2,
                    center.v2,
                    skinColor,
                    clip);
        ++drawCalls;
    }
    else
    {
        if (border.left && border.top)
        {
            batch->draw(_absoluteBounds.x,
                        _absoluteBounds.y,
                        border.left,
                        border.top,
                        topLeft.u1,
                        topLeft.v1,
                        topLeft.u2,
                        topLeft.v2,
                        skinColor,
                        clip);
            ++drawCalls;
        }
        if (border.top)
        {
            batch->draw(_absoluteBounds.x + border.left,
                        _absoluteBounds.y,
                        midWidth,
                        border.top,
                        top.u1,
                        top.v1,
                        top.u2,
                        top.v2,
                        skinColor,
                        clip);
            ++drawCalls;
        }
        if (border.right && border.top)
        {
            batch->draw(rightX,
                        _absoluteBounds.y,
                        border.right,
                        border.top,
                        topRight.u1,
                        topRight.v1,
                        topRight.u2,
                        topRight.v2,
                        skinColor,
                        clip);
            ++drawCalls;
        }
        if (border.left)
        {
            batch->draw(_absoluteBounds.x,
                        midY,
                        border.left,
                        midHeight,
                        left.u1,
                        left.v1,
                        left.u2,
                        left.v2,
                        skinColor,
                        clip);
            ++drawCalls;
        }

        // Always draw the background.
        batch->draw(_absoluteBounds.x + border.left,
                    _absoluteBounds.y + border.top,
                    _absoluteBounds.width - border.left - border.right,
                    _absoluteBounds.height - border.top - border.bottom,
                    center.u1,
                    center.v1,
                    center.u2,
                    center.v2,
                    skinColor,
                    clip);
        ++drawCalls;

        if (border.right)
        {
            batch->draw(rightX,
                        midY,
                        border.right,
                        midHeight,
                        right.u1,
                        right.v1,
                        right.u2,
                        right.v2,
                        skinColor,
                        clip);
            ++drawCalls;
        }
        if (border.bottom && border.left)
        {
            batch->draw(_absoluteBounds.x,
                        bottomY,
                        border.left,
                        border.bottom,
                        bottomLeft.u1,
                        bottomLeft.v1,
                        bottomLeft.u2,
                        bottomLeft.v2,
                        skinColor,
                        clip);
            ++drawCalls;
        }
        if (border.bottom)
        {
            batch->draw(midX,
                        bottomY,
                        midWidth,
                        border.bottom,
                        bottom.u1,
                        bottom.v1,
                        bottom.u2,
                        bottom.v2,
                        skinColor,
                        clip);
            ++drawCalls;
        }
        if (border.bottom && border.right)
        {
            batch->draw(rightX,
                        bottomY,
                        border.right,
                        border.bottom,
                        bottomRight.u1,
                        bottomRight.v1,
                        bottomRight.u2,
                        bottomRight.v2,
                        skinColor,
                        clip);
            ++drawCalls;
        }
    }

    finishBatch(form, batch);

    return drawCalls;
}

Control::State Control::getState(const std::string& state)
{
    if (state.empty())
    {
        return NORMAL;
    }

    if (state.compare("NORMAL") == 0)
    {
        return NORMAL;
    }
    else if (state.compare("ACTIVE") == 0)
    {
        return ACTIVE;
    }
    else if (state.compare("FOCUS") == 0)
    {
        return FOCUS;
    }
    else if (state.compare("DISABLED") == 0)
    {
        return DISABLED;
    }
    else if (state.compare("HOVER") == 0)
    {
        return HOVER;
    }

    return NORMAL;
}

Theme::ThemeImage* Control::getImage(const std::string& id, State state)
{
    Theme::ThemeImage* image = nullptr;

    Theme::Style::Overlay* overlay = getOverlay(state);
    if (overlay)
    {
        Theme::ImageList* imageList = overlay->getImageList();
        if (imageList) image = imageList->getImage(id);
    }

    return image ? image : _style->getTheme()->_emptyImage;
}

Control* Control::getParent() const noexcept { return _parent; }

bool Control::isChild(Control* control) const
{
    if (!control) return false;

    Control* parent = _parent;
    while (parent)
    {
        if (parent == control) return true;
        parent = parent->_parent;
    }

    return false;
}

Form* Control::getTopLevelForm() const
{
    if (_parent) return _parent->getTopLevelForm();

    if (isContainer())
    {
        Container* container = static_cast<Container*>(const_cast<Control*>(this));
        if (container->isForm()) return static_cast<Form*>(container);
    }

    return nullptr;
}

// Implementation of AnimationHandler
unsigned int Control::getAnimationPropertyComponentCount(int propertyId) const
{
    switch (propertyId)
    {
        case ANIMATE_POSITION:
        case ANIMATE_SIZE:
            return 2;

        case ANIMATE_POSITION_X:
        case ANIMATE_POSITION_Y:
        case ANIMATE_SIZE_WIDTH:
        case ANIMATE_SIZE_HEIGHT:
        case ANIMATE_OPACITY:
            return 1;

        default:
            return -1;
    }
}

void Control::getAnimationPropertyValue(int propertyId, AnimationValue* value)
{
    assert(value);

    switch (propertyId)
    {
        case ANIMATE_POSITION:
            value->setFloat(0, _bounds.x);
            value->setFloat(1, _bounds.y);
            break;
        case ANIMATE_SIZE:
            value->setFloat(0, _bounds.width);
            value->setFloat(1, _bounds.height);
            break;
        case ANIMATE_POSITION_X:
            value->setFloat(0, _bounds.x);
            break;
        case ANIMATE_POSITION_Y:
            value->setFloat(0, _bounds.y);
            break;
        case ANIMATE_SIZE_WIDTH:
            value->setFloat(0, _bounds.width);
            break;
        case ANIMATE_SIZE_HEIGHT:
            value->setFloat(0, _bounds.height);
            break;
        case ANIMATE_OPACITY:
            value->setFloat(0, _opacity);
            break;
        default:
            break;
    }
}

void Control::setAnimationPropertyValue(int propertyId, AnimationValue* value, float blendWeight)
{
    assert(value);

    switch (propertyId)
    {
        case ANIMATE_POSITION:
            setX(Curve::lerp(blendWeight, _bounds.x, value->getFloat(0)), isXPercentage());
            setY(Curve::lerp(blendWeight, _bounds.y, value->getFloat(1)), isYPercentage());
            break;
        case ANIMATE_POSITION_X:
            setX(Curve::lerp(blendWeight, _bounds.x, value->getFloat(0)), isXPercentage());
            break;
        case ANIMATE_POSITION_Y:
            setY(Curve::lerp(blendWeight, _bounds.y, value->getFloat(0)), isYPercentage());
            break;
        case ANIMATE_SIZE:
            setWidth(Curve::lerp(blendWeight, _bounds.width, value->getFloat(0)),
                     isWidthPercentage());
            setHeight(Curve::lerp(blendWeight, _bounds.height, value->getFloat(1)),
                      isHeightPercentage());
            break;
        case ANIMATE_SIZE_WIDTH:
            setWidth(Curve::lerp(blendWeight, _bounds.width, value->getFloat(0)),
                     isWidthPercentage());
            break;
        case ANIMATE_SIZE_HEIGHT:
            setHeight(Curve::lerp(blendWeight, _bounds.height, value->getFloat(0)),
                      isHeightPercentage());
            break;
        case ANIMATE_OPACITY:
            setOpacity(Curve::lerp(blendWeight, _opacity, value->getFloat(0)));
            break;
    }
}

Theme::Style::Overlay** Control::getOverlays(unsigned char overlayTypes,
                                             Theme::Style::Overlay** overlays)
{
    assert(overlays);
    assert(_style);

    unsigned int index = 0;
    if ((overlayTypes & NORMAL) == NORMAL)
    {
        overlays[index++] = _style->getOverlay(Theme::Style::OVERLAY_NORMAL);
    }

    if ((overlayTypes & FOCUS) == FOCUS)
    {
        overlays[index++] = _style->getOverlay(Theme::Style::OVERLAY_FOCUS);
    }

    if ((overlayTypes & ACTIVE) == ACTIVE)
    {
        overlays[index++] = _style->getOverlay(Theme::Style::OVERLAY_ACTIVE);
    }

    if ((overlayTypes & DISABLED) == DISABLED)
    {
        overlays[index++] = _style->getOverlay(Theme::Style::OVERLAY_DISABLED);
    }

    if ((overlayTypes & HOVER) == HOVER)
    {
        overlays[index++] = _style->getOverlay(Theme::Style::OVERLAY_HOVER);
    }

    return overlays;
}

Theme::Style::Overlay* Control::getOverlay(State state) const noexcept
{
    assert(_style);

    Theme::Style::Overlay* overlay = nullptr;

    switch (state)
    {
        case Control::NORMAL:
            return _style->getOverlay(Theme::Style::OVERLAY_NORMAL);

        case Control::FOCUS:
            overlay = _style->getOverlay(Theme::Style::OVERLAY_FOCUS);
            break;

        case Control::ACTIVE:
            overlay = _style->getOverlay(Theme::Style::OVERLAY_ACTIVE);
            if (!overlay && hasFocus()) overlay = _style->getOverlay(Theme::Style::OVERLAY_FOCUS);
            break;

        case Control::DISABLED:
            overlay = _style->getOverlay(Theme::Style::OVERLAY_DISABLED);
            break;

        case Control::HOVER:
            overlay = _style->getOverlay(Theme::Style::OVERLAY_HOVER);
            if (!overlay && hasFocus()) overlay = _style->getOverlay(Theme::Style::OVERLAY_FOCUS);
            break;
    }

    // Fall back to normal overlay if more specific state overlay not found
    if (!overlay) overlay = _style->getOverlay(Theme::Style::OVERLAY_NORMAL);

    return overlay;
}

void Control::overrideStyle()
{
    if (_styleOverridden)
    {
        return;
    }

    // Copy the style.
    assert(_style);
    _style = new Theme::Style(*_style);
    _styleOverridden = true;
}

void Control::overrideThemedProperties(Properties* properties, unsigned char states)
{
    assert(properties);
    assert(_style);
    assert(_style->_theme);

    Theme::ImageList* imageList = nullptr;
    Theme::ThemeImage* cursor = nullptr;
    Theme::Skin* skin = nullptr;
    _style->_theme->lookUpSprites(properties, &imageList, &cursor, &skin);

    if (imageList)
    {
        setImageList(imageList, states);
    }

    if (cursor)
    {
        setCursor(cursor, states);
    }

    if (skin)
    {
        setSkin(skin, states);
    }

    if (properties->exists("font"))
    {
        Font* font = Font::create(properties->getString("font"));
        setFont(font, states);
        font->release();
    }

    if (properties->exists("fontSize"))
    {
        setFontSize(properties->getInt("fontSize"), states);
    }

    if (properties->exists("textColor"))
    {
        Vector4 textColor(0, 0, 0, 1);
        properties->getColor("textColor", &textColor);
        setTextColor(textColor, states);
    }

    if (properties->exists("textAlignment"))
    {
        setTextAlignment(Font::getJustify(properties->getString("textAlignment")), states);
    }

    if (properties->exists("rightToLeft"))
    {
        setTextRightToLeft(properties->getBool("rightToLeft"), states);
    }
    if (properties->exists("opacity"))
    {
        setOpacity(properties->getFloat("opacity"), states);
    }
}

void Control::setImageList(Theme::ImageList* imageList, unsigned char states)
{
    overrideStyle();
    Theme::Style::Overlay* overlays[Theme::Style::OVERLAY_MAX] = { 0 };
    getOverlays(states, overlays);

    for (size_t i = 0; i < Theme::Style::OVERLAY_MAX; ++i)
    {
        if (overlays[i]) overlays[i]->setImageList(imageList);
    }
    if (_autoSize != AUTO_SIZE_NONE) setDirty(DIRTY_BOUNDS);
}

void Control::setCursor(Theme::ThemeImage* cursor, unsigned char states)
{
    overrideStyle();
    Theme::Style::Overlay* overlays[Theme::Style::OVERLAY_MAX] = { 0 };
    getOverlays(states, overlays);

    for (size_t i = 0; i < Theme::Style::OVERLAY_MAX; ++i)
    {
        if (overlays[i]) overlays[i]->setCursor(cursor);
    }
}

void Control::setSkin(Theme::Skin* skin, unsigned char states)
{
    overrideStyle();
    Theme::Style::Overlay* overlays[Theme::Style::OVERLAY_MAX] = { 0 };
    getOverlays(states, overlays);

    for (size_t i = 0; i < Theme::Style::OVERLAY_MAX; ++i)
    {
        if (overlays[i]) overlays[i]->setSkin(skin);
    }
    if (_autoSize != AUTO_SIZE_NONE) setDirty(DIRTY_BOUNDS);
}

Theme::Skin* Control::getSkin(State state)
{
    Theme::Style::Overlay* overlay = getOverlay(state);
    assert(overlay);
    return overlay->getSkin();
}

Control::Alignment Control::getAlignment(const std::string& alignment)
{
    if (alignment.empty())
    {
        return Control::ALIGN_TOP_LEFT;
    }

    if (alignment == "ALIGN_LEFT")
    {
        return Control::ALIGN_LEFT;
    }
    else if (alignment == "ALIGN_HCENTER")
    {
        return Control::ALIGN_HCENTER;
    }
    else if (alignment == "ALIGN_RIGHT")
    {
        return Control::ALIGN_RIGHT;
    }
    else if (alignment == "ALIGN_TOP")
    {
        return Control::ALIGN_TOP;
    }
    else if (alignment == "ALIGN_VCENTER")
    {
        return Control::ALIGN_VCENTER;
    }
    else if (alignment == "ALIGN_BOTTOM")
    {
        return Control::ALIGN_BOTTOM;
    }
    else if (alignment == "ALIGN_TOP_LEFT")
    {
        return Control::ALIGN_TOP_LEFT;
    }
    else if (alignment == "ALIGN_VCENTER_LEFT")
    {
        return Control::ALIGN_VCENTER_LEFT;
    }
    else if (alignment == "ALIGN_BOTTOM_LEFT")
    {
        return Control::ALIGN_BOTTOM_LEFT;
    }
    else if (alignment == "ALIGN_TOP_HCENTER")
    {
        return Control::ALIGN_TOP_HCENTER;
    }
    else if (alignment == "ALIGN_VCENTER_HCENTER")
    {
        return Control::ALIGN_VCENTER_HCENTER;
    }
    else if (alignment == "ALIGN_BOTTOM_HCENTER")
    {
        return Control::ALIGN_BOTTOM_HCENTER;
    }
    else if (alignment == "ALIGN_TOP_RIGHT")
    {
        return Control::ALIGN_TOP_RIGHT;
    }
    else if (alignment == "ALIGN_VCENTER_RIGHT")
    {
        return Control::ALIGN_VCENTER_RIGHT;
    }
    else if (alignment == "ALIGN_BOTTOM_RIGHT")
    {
        return Control::ALIGN_BOTTOM_RIGHT;
    }
    else
    {
        GP_ERROR("Failed to get corresponding control alignment for unsupported value '%s'.",
                 alignment);
    }
    return Control::ALIGN_TOP_LEFT;
}

float Control::parseCoord(const std::string& s, bool* isPercentage)
{
    if (auto pos = s.find('%'); pos != std::string::npos)
    {
        *isPercentage = true;
        return std::stof(s.substr(0, pos)) * 0.01f;
    }
    *isPercentage = false;
    return std::stof(s);
}

//----------------------------------------------------------------
bool Control::parseCoordPair(const std::string& s,
                             float* v1,
                             float* v2,
                             bool* v1Percentage,
                             bool* v2Percentage)
{
    auto delimiterPos = s.find(',');
    if (delimiterPos == std::string::npos) return false;

    std::string v1Str = s.substr(0, delimiterPos);
    std::string v2Str = s.substr(delimiterPos + 1);

    *v1 = parseCoord(v1Str, v1Percentage);
    *v2 = parseCoord(v2Str, v2Percentage);

    return true;
}
} // namespace tractor

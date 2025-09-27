#include "pch.h"

#include "ui/Label.h"

namespace tractor
{

Label::Label() : _text(""), _font(nullptr) {}

Label::~Label() {}

Label* Label::create(const std::string& id, Theme::Style* style)
{
    Label* label = new Label();
    label->_id = id;
    label->initialize("Label", style, nullptr);
    return label;
}

Control* Label::create(Theme::Style* style, Properties* properties)
{
    Label* label = new Label();
    label->initialize("Label", style, properties);
    return label;
}

void Label::initialize(const std::string& typeName, Theme::Style* style, Properties* properties)
{
    Control::initialize(typeName, style, properties);

    if (properties)
    {
        const auto& text = properties->getString("text");
        if (!text.empty())
        {
            _text = text;
        }
    }
}

const std::string& Label::getTypeName() const
{
    static const std::string TYPE_NAME = "Label";
    return TYPE_NAME;
}

void Label::addListener(Control::Listener* listener, int eventFlags)
{
    if ((eventFlags & Control::Listener::TEXT_CHANGED) == Control::Listener::TEXT_CHANGED)
    {
        GP_ERROR("TEXT_CHANGED event is not applicable to this control.");
    }
    if ((eventFlags & Control::Listener::VALUE_CHANGED) == Control::Listener::VALUE_CHANGED)
    {
        GP_ERROR("VALUE_CHANGED event is not applicable to this control.");
    }

    Control::addListener(listener, eventFlags);
}

void Label::setText(const std::string& text)
{
    if (text != _text)
    {
        _text = text;
        if (_autoSize != AUTO_SIZE_NONE) setDirty(DIRTY_BOUNDS);
    }
}

const std::string& Label::getText() { return _text; }

void Label::update(float elapsedTime)
{
    Control::update(elapsedTime);

    // Update text opacity each frame since opacity is updated in Control::update.
    _textColor = getTextColor(getState());
    _textColor.w *= _opacity;
}

void Label::updateState(State state)
{
    Control::updateState(state);

    _font = getFont(state);
}

void Label::updateBounds()
{
    Control::updateBounds();

    if (_autoSize != AUTO_SIZE_NONE && _font)
    {
        // Measure bounds based only on normal state so that bounds updates are not always required
        // on state changes. This is a trade-off for functionality vs performance, but changing the
        // size of UI controls on hover/focus/etc is a pretty bad practice so we'll prioritize
        // performance here.
        unsigned int w, h;
        _font->measureText(_text, getFontSize(NORMAL), &w, &h);
        if (_autoSize & AUTO_SIZE_WIDTH)
        {
            setWidthInternal(w + getBorder(NORMAL).left + getBorder(NORMAL).right
                             + getPadding().left + getPadding().right);
        }
        if (_autoSize & AUTO_SIZE_HEIGHT)
        {
            setHeightInternal(h + getBorder(NORMAL).top + getBorder(NORMAL).bottom
                              + getPadding().top + getPadding().bottom);
        }
    }
}

void Label::updateAbsoluteBounds(const Vector2& offset)
{
    Control::updateAbsoluteBounds(offset);

    _textBounds.set((int)_viewportBounds.x,
                    (int)_viewportBounds.y,
                    _viewportBounds.width,
                    _viewportBounds.height);
}

unsigned int Label::drawText(Form* form, const Rectangle& clip)
{
    // Draw the text.
    if (_text.size() > 0 && _font)
    {
        Control::State state = getState();
        unsigned int fontSize = getFontSize(state);

        SpriteBatch* batch = _font->getSpriteBatch(fontSize);
        startBatch(form, batch);
        _font->drawText(_text,
                        _textBounds,
                        _textColor,
                        fontSize,
                        getTextAlignment(state),
                        true,
                        getTextRightToLeft(state),
                        _viewportClipBounds);
        finishBatch(form, batch);

        return 1;
    }

    return 0;
}

} // namespace tractor

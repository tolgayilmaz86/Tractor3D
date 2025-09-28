#include "pch.h"

#include "ui/Button.h"

#include "input/Gamepad.h"

namespace tractor
{
Button* Button::create(const std::string& id, Theme::Style* style)
{
    Button* button = new Button();
    button->_id = id;
    button->initialize("Button", style, nullptr);
    return button;
}

Control* Button::create(Theme::Style* style, Properties* properties)
{
    Button* button = new Button();
    button->initialize("Button", style, properties);
    return button;
}

void Button::initialize(const std::string& typeName, Theme::Style* style, Properties* properties)
{
    Label::initialize(typeName, style, properties);

    if (properties)
    {
        // Different types of data bindings can be named differently in a button namespace.
        // Gamepad button mappings have the name "mapping" and correspond to Gamepad::ButtonMapping enums.
        const auto& mapping = properties->getString("mapping");
        if (!mapping.empty())
        {
            _dataBinding = Gamepad::getButtonMappingFromString(mapping);
        }
    }
}

const std::string& Button::getTypeName() const
{
    static const std::string TYPE_NAME = "Button";
    return TYPE_NAME;
}
} // namespace tractor

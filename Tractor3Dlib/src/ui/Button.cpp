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

const std::string& Button::getTypeName() const noexcept
{
    static const std::string TYPE_NAME = "Button";
    return TYPE_NAME;
}
} // namespace tractor

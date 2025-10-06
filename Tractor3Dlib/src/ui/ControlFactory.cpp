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

#include "ui/ControlFactory.h"

#include "input/ImageControl.h"
#include "input/JoystickControl.h"
#include "ui/Button.h"
#include "ui/CheckBox.h"
#include "ui/Container.h"
#include "ui/Label.h"
#include "ui/RadioButton.h"
#include "ui/Slider.h"
#include "ui/TextBox.h"

namespace tractor
{

static ControlFactory* __controlFactory = nullptr;

//----------------------------------------------------------------
ControlFactory::ControlFactory() { registerStandardControls(); }

//----------------------------------------------------------------
void ControlFactory::finalize() { SAFE_DELETE(__controlFactory); }

//----------------------------------------------------------------
ControlFactory* ControlFactory::getInstance()
{
    if (__controlFactory == nullptr) __controlFactory = new ControlFactory();
    return __controlFactory;
}

//----------------------------------------------------------------
bool ControlFactory::registerCustomControl(const std::string& typeName, ControlActivator activator)
{
    std::string upper(typeName);
    std::transform(upper.begin(), upper.end(), upper.begin(), (int (*)(int))toupper);

    if (_registeredControls.find(upper) != _registeredControls.end()) return false;

    _registeredControls[upper] = activator;
    return true;
}

//----------------------------------------------------------------
void ControlFactory::unregisterCustomControl(const std::string& typeName)
{
    std::string upper(typeName);
    std::transform(upper.begin(), upper.end(), upper.begin(), (int (*)(int))toupper);

    std::map<std::string, ControlActivator>::iterator it;
    if ((it = _registeredControls.find(upper)) != _registeredControls.end())
    {
        _registeredControls.erase(it);
    }
}

//----------------------------------------------------------------
Control* ControlFactory::createControl(const std::string& typeName,
                                       Theme::Style* style,
                                       Properties* properties)
{
    std::string upper(typeName);
    std::transform(upper.begin(), upper.end(), upper.begin(), (int (*)(int))toupper);

    std::map<std::string, ControlActivator>::iterator it = _registeredControls.find(upper);
    if (it == _registeredControls.end()) return nullptr;

    return (*it->second)(style, properties);
}

//----------------------------------------------------------------
void ControlFactory::registerStandardControls()
{
    registerCustomControl("LABEL", &Label::create);
    registerCustomControl("BUTTON", &Button::create);
    registerCustomControl("CHECKBOX", &CheckBox::create);
    registerCustomControl("RADIOBUTTON", &RadioButton::create);
    registerCustomControl("CONTAINER", &Container::create);
    registerCustomControl("SLIDER", &Slider::create);
    registerCustomControl("TEXTBOX", &TextBox::create);
    registerCustomControl("JOYSTICK", &JoystickControl::create); // convenience alias
    registerCustomControl("JOYSTICKCONTROL", &JoystickControl::create);
    registerCustomControl("IMAGE", &ImageControl::create); // convenience alias
    registerCustomControl("IMAGECONTROL", &ImageControl::create);
}

} // namespace tractor

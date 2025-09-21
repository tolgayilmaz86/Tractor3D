#include "framework/Base.h"

#include "ui/ControlFactory.h"

#include "ui/Label.h"
#include "ui/Button.h"
#include "ui/CheckBox.h"
#include "ui/RadioButton.h"
#include "ui/Container.h"
#include "ui/Slider.h"
#include "ui/TextBox.h"
#include "input/JoystickControl.h"
#include "input/ImageControl.h"

namespace tractor
{

  static ControlFactory* __controlFactory = nullptr;

  ControlFactory::ControlFactory()
  {
    registerStandardControls();
  }

  ControlFactory::ControlFactory(const ControlFactory& copy)
  {
  }

  ControlFactory::~ControlFactory()
  {
  }

  void ControlFactory::finalize()
  {
    SAFE_DELETE(__controlFactory);
  }

  ControlFactory* ControlFactory::getInstance()
  {
    if (__controlFactory == nullptr)
      __controlFactory = new ControlFactory();
    return __controlFactory;
  }

  bool ControlFactory::registerCustomControl(const char* typeName, ControlActivator activator)
  {
    std::string upper(typeName);
    std::transform(upper.begin(), upper.end(), upper.begin(), (int(*)(int))toupper);

    if (_registeredControls.find(upper) != _registeredControls.end())
      return false;

    _registeredControls[upper] = activator;
    return true;
  }

  void ControlFactory::unregisterCustomControl(const char* typeName)
  {
    std::string upper(typeName);
    std::transform(upper.begin(), upper.end(), upper.begin(), (int(*)(int))toupper);

    std::map<std::string, ControlActivator>::iterator it;
    if ((it = _registeredControls.find(upper)) != _registeredControls.end())
    {
      _registeredControls.erase(it);
    }
  }

  Control* ControlFactory::createControl(const char* typeName, Theme::Style* style, Properties* properties)
  {
    std::string upper(typeName);
    std::transform(upper.begin(), upper.end(), upper.begin(), (int(*)(int))toupper);

    std::map<std::string, ControlActivator>::iterator it = _registeredControls.find(upper);
    if (it == _registeredControls.end())
      return nullptr;

    return (*it->second)(style, properties);
  }

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
    registerCustomControl("IMAGE", &ImageControl::create);  // convenience alias
    registerCustomControl("IMAGECONTROL", &ImageControl::create);
  }

}

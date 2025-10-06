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

#include "input/Touch.h"
#include "scene/Properties.h"
#include "ui/Container.h"
#include "ui/Label.h"
#include "ui/Theme.h"

namespace tractor
{

/**
 * Defines a button control.
 */
class Button : public Label
{
    friend class Container;
    friend class Gamepad;
    friend class ControlFactory;

  public:
    /**
     * Creates a new Button.
     *
     * @param id The button ID.
     * @param style The button style (optional).
     *
     * @return The new button.
     * @script{create}
     */
    static Button* create(const std::string& id, Theme::Style* style = nullptr);

  protected:
    /**
     * Constructor.
     */
    Button() = default;

    /**
     * Destructor.
     */
    virtual ~Button() = default;

    /**
     * Create a button with a given style and properties.
     *
     * @param style The style to apply to this button.
     * @param properties A properties object containing a definition of the button (optional).
     *
     * @return The new button.
     */
    static Control* create(Theme::Style* style, Properties* properties = nullptr);

    /**
     * @see Control::initialize
     */
    void initialize(const std::string& typeName, Theme::Style* style, Properties* properties);

    /**
     * Extends ScriptTarget::getTypeName() to return the type name of this class.
     *
     * Child controls should override this function to return the correct type name.
     *
     * @return The type name of this class: "Button"
     * @see ScriptTarget::getTypeName()
     */
    const std::string& getTypeName() const noexcept;

    /**
     * Gets the data binding index for this control.
     *
     * @return The data binding index for control.
     */
    const unsigned int getDataBinding() const noexcept { return _dataBinding; }

    /**
     * Sets the data binding provider for this control.
     *
     * @param dataBinding The data binding index for control.
     */
    void setDataBinding(unsigned int dataBinding) { _dataBinding = dataBinding; }

  private:
    /**
     * Constructor.
     */
    Button(const Button& copy);

    unsigned int _dataBinding;
};

} // namespace tractor

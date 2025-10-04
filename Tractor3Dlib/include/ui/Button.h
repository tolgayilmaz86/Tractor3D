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

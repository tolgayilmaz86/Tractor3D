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

#include "ui/Control.h"
#include "ui/Theme.h"

namespace tractor
{

/**
 * Defines a label control.
 *
 * This is capable of rendering text within its border.
 */
class Label : public Control
{
    friend class Container;
    friend class ControlFactory;

  public:
    /**
     * Creates a new label.
     *
     * @param id The label id.
     * @param style The label style (optional).
     *
     * @return The new label.
     * @script{create}
     */
    static Label* create(const std::string& id, Theme::Style* style = nullptr);

    /**
     * Extends ScriptTarget::getTypeName() to return the type name of this class.
     *
     * Child controls should override this function to return the correct type name.
     *
     * @return The type name of this class: "Label"
     * @see ScriptTarget::getTypeName()
     */
    const std::string& getTypeName() const noexcept;

    /**
     * Set the text for this label to display.
     *
     * @param text The text to display.
     */
    virtual void setText(const std::string& text);

    /**
     * Get the text displayed by this label.
     *
     * @return The text displayed by this label.
     */
    const std::string& getText();

    /**
     * Add a listener to be notified of specific events affecting
     * this control.  Event types can be OR'ed together.
     * E.g. To listen to touch-press and touch-release events,
     * pass <code>Control::Listener::TOUCH | Control::Listener::RELEASE</code>
     * as the second parameter.
     *
     * @param listener The listener to add.
     * @param eventFlags The events to listen for.
     */
    virtual void addListener(Control::Listener* listener, int eventFlags);

  protected:
    /**
     * Constructor.
     */
    Label();

    /**
     * Destructor.
     */
    virtual ~Label();

    /**
     * Create a new label control.
     *
     * @param style The control's custom style.
     * @param properties A properties object containing a definition of the label (optional).
     *
     * @return The new label.
     * @script{create}
     */
    static Control* create(Theme::Style* style, Properties* properties);

    /**
     * @see Control::initialize
     */
    void initialize(const std::string& typeName, Theme::Style* style, Properties* properties);

    /**
     * @see Control::update
     */
    void update(float elapsedTime);

    /**
     * @see Control::updateState
     */
    void updateState(State state);

    /**
     * @see Control::updateBounds
     */
    void updateBounds();

    /**
     * @see Control::updateAbsoluteBounds
     */
    void updateAbsoluteBounds(const Vector2& offset);

    /**
     * @see Control::drawText
     */
    virtual unsigned int drawText(Form* form, const Rectangle& clip);

    /**
     * The text displayed by this label.
     */
    std::string _text{};

    /**
     * The font being used to display the label.
     */
    Font* _font{ nullptr };

    /**
     * The text color being used to display the label.
     */
    Vector4 _textColor{ 1.0f, 1.0f, 1.0f, 1.0f };

    /**
     * The position and size of this control's text area, before clipping.  Used for text alignment.
     */
    Rectangle _textBounds{};

  private:
    /**
     * Constructor.
     */
    Label(const Label& copy);
};

} // namespace tractor

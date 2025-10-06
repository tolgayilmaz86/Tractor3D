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

#include <string>

#include "ui/Label.h"

namespace tractor
{

/**
 * Defines a text control.
 *
 * Listeners can listen for a TEXT_CHANGED event, and then query the text box
 * for the last keypress it received.
 * On mobile device you can tap or click within the text box to
 * bring up the virtual keyboard.
 */
class TextBox : public Label
{
    friend class Container;
    friend class ControlFactory;

  public:
    /**
     * Input modes. Default is Text.
     */
    enum InputMode
    {
        /**
         * Text: Text is displayed directly.
         */
        TEXT = 0x01,

        /**
         * Password: Text is replaced by _passwordChar, which is '*' by default.
         */
        PASSWORD = 0x02
    };

    /**
     * Creates a new TextBox.
     *
     * @param id The textbox ID.
     * @param style The textbox style (optional).
     *
     * @return The new textbox.
     * @script{create}
     */
    static TextBox* create(const std::string& id, Theme::Style* style = nullptr);

    /**
     * Extends ScriptTarget::getTypeName() to return the type name of this class.
     *
     * Child controls should override this function to return the correct type name.
     *
     * @return The type name of this class: "TextBox"
     * @see ScriptTarget::getTypeName()
     */
    const std::string& getTypeName() const noexcept;

    /**
     * Returns the current location of the caret with the text of this TextBox.
     *
     * @return The current caret location.
     */
    unsigned int getCaretLocation() const noexcept { return _caretLocation; }

    /**
     * Sets the location of the caret within this text box.
     *
     * @param index The new location of the caret within the text of this TextBox.
     */
    void setCaretLocation(unsigned int index);

    /**
     * Get the last key pressed within this text box.
     *
     * @return The last key pressed within this text box.
     */
    int getLastKeypress() const noexcept { return _lastKeypress; }

    /**
     * Set the character displayed in password mode.
     *
     * @param character Character to display in password mode.
     */
    void setPasswordChar(char character) { _passwordChar = character; }

    /**
     * Get the character displayed in password mode.
     *
     * @return The character displayed in password mode.
     */
    char getPasswordChar() const noexcept { return _passwordChar; }

    /**
     * Set the input mode.
     *
     * @param inputMode Input mode to set.
     */
    void setInputMode(InputMode inputMode) { _inputMode = inputMode; }

    /**
     * Get the input mode.
     *
     * @return The input mode.
     */
    InputMode getInputMode() const noexcept { return _inputMode; }

    /**
     * @see Control::addListener
     */
    virtual void addListener(Control::Listener* listener, int eventFlags);

    /**
     * Update the text being edited.
     */
    void setText(const std::string& text) override;

  protected:
    /**
     * Constructor.
     */
    TextBox() = default;

    /**
     * Destructor.
     */
    virtual ~TextBox() = default;

    /**
     * Create a text box with a given style and properties.
     *
     * @param style The style to apply to this text box.
     * @param properties A properties object containing a definition of the text box (optional).
     *
     * @return The new text box.
     */
    static Control* create(Theme::Style* style, Properties* properties = nullptr);

    /**
     * @see Control::initialize
     */
    void initialize(const std::string& typeName, Theme::Style* style, Properties* properties);

    /**
     * Touch callback on touch events.  Controls return true if they consume the touch event.
     *
     * @param evt The touch event that occurred.
     * @param x The x position of the touch in pixels. Left edge is zero.
     * @param y The y position of the touch in pixels. Top edge is zero.
     * @param contactIndex The order of occurrence for multiple touch contacts starting at zero.
     *
     * @return Whether the touch event was consumed by the control.
     *
     * @see Touch::TouchEvent
     */
    bool touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

    /**
     * Keyboard callback on key events.
     *
     * @param evt The key event that occurred.
     * @param key If evt is KEY_PRESS or KEY_RELEASE then key is the key code from Keyboard::Key.
     *            If evt is KEY_CHAR then key is the unicode value of the character.
     *
     * @see Keyboard::KeyEvent
     * @see Keyboard::Key
     */
    bool keyEvent(Keyboard::KeyEvent evt, int key);

    /**
     * @see Control#controlEvent
     */
    void controlEvent(Control::Listener::EventType evt);

    /**
     * @see Control::updateState
     */
    void updateState(State state);

    /**
     * @see Control::drawImages
     */
    unsigned int drawImages(Form* form, const Rectangle& clip);

    /**
     * @see Control::drawText
     */
    unsigned int drawText(Form* form, const Rectangle& clip);

    /**
     * Gets an InputMode by string.
     *
     * @param inputMode The string representation of the InputMode type.
     * @return The InputMode enum value corresponding to the given string.
     */
    static InputMode getInputMode(const std::string& inputMode);

    /**
     * Get the text which should be displayed, depending on
     * _inputMode.
     *
     * @return The text to be displayed.
     */
    std::string getDisplayedText() const;

    /**
     * The current location of the TextBox's caret.
     */
    unsigned int _caretLocation;

    /**
     * The previous position of the TextBox's caret.
     */
    Vector2 _prevCaretLocation;

    /**
     * The last character that was entered into the TextBox.
     */
    int _lastKeypress;

    /**
     * The font size to be used in the TextBox.
     */
    unsigned int _fontSize;

    /**
     * The Theme::Image for the TextBox's caret.
     */
    Theme::ThemeImage* _caretImage;

    /**
     * The character displayed in password mode.
     */
    char _passwordChar{'*'};

    /**
     * The mode used to display the typed text.
     */
    InputMode _inputMode;

    /**
     * Indicate if the CTRL key is currently pressed.
     */
    bool _ctrlPressed;

    /**
     * Indicate if the SHIFT key is currently pressed.
     */
    bool _shiftPressed = false;

  private:
    /**
     * Constructor.
     */
    TextBox(const TextBox& copy);

    void setCaretLocation(int x, int y);

    void getCaretLocation(Vector2* p);
};

} // namespace tractor

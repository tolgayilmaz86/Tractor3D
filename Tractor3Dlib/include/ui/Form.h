#pragma once

#include "graphics/Drawable.h"
#include "graphics/Mesh.h"
#include "input/Gamepad.h"
#include "input/Keyboard.h"
#include "input/Mouse.h"
#include "input/Touch.h"
#include "renderer/FrameBuffer.h"
#include "scene/Node.h"
#include "ui/Container.h"
#include "utils/Ref.h"

namespace tractor
{

class Theme;

/**
 * Defines a form that is a root container that contains zero or more controls.
 *
 * This can also be attached on a scene Node to support 3D forms.
 */
class Form : public Drawable, public Container
{
    friend class Platform;
    friend class Game;
    friend class Gamepad;
    friend class Control;
    friend class Container;

  public:
    /**
     * Creates a form from a .form properties file.
     *
     * @param url The URL pointing to the Properties object defining the form.
     *
     * @return The new form or nullptr if there was an error.
     * @script{create}
     */
    static Form* create(const std::string& url);

    /**
     * Create a new Form.
     *
     * The specified style defines the visual style for the form. If nullptr is passed
     * for the style, the default UI theme is used. All controls attached to this
     * form will inherit the theme that contains the form's style.
     *
     * @param id The Form's ID.
     * @param style The Form's custom style (optional - may be nullptr).
     * @param layoutType The form's layout type (optional).
     *
     * @return The new Form.
     * @script{create}
     */
    static Form* create(const std::string& id,
                        Theme::Style* style,
                        Layout::Type layoutType = Layout::LAYOUT_ABSOLUTE);

    /**
     * Get a form from its ID.
     *
     * @param id The ID of the form to search for.
     *
     * @return A form with the given ID, or null if one was not found.
     */
    static Form* getForm(const std::string& id);

    /**
     * Returns the currently active control within the UI system.
     *
     * An active control is a control that is currently pressed or hovered over. On a multi-touch
     * system, it is possible for several controls to be active at once (one for each touch point).
     * However, only a single control can have focus at once.
     *
     * @param touchIndex Optional touch point index to retrieve the active control for.
     *
     * @return The currently active control, or nullptr if no controls are currently active.
     */
    static Control* getActiveControl(unsigned int touchIndex = 0);

    /**
     * Returns the current control that is in focus.
     *
     * @return The current control in focus, or nullptr if no controls are in focus.
     */
    static Control* getFocusControl();

    /**
     * Removes focus from any currently focused UI control.
     */
    static void clearFocus();

    /**
     * Extends ScriptTarget::getTypeName() to return the type name of this class.
     *
     * Child controls should override this function to return the correct type name.
     *
     * @return The type name of this class: "Form"
     * @see ScriptTarget::getTypeName
     */
    const std::string& getTypeName() const;

    /**
     * @see Container#isForm()
     */
    bool isForm() const noexcept { return true; }

    /**
     * @see Control::update
     */
    void update(float elapsedTime);

    /**
     * Draws this form.
     *
     * @return The nubmer of draw calls issued to draw the form.
     */
    unsigned int draw(bool wireframe = false);

    /**
     * Determines whether batching is enabled for this form.
     *
     * @return True if batching is enabled for this form, false otherwise.
     */
    bool isBatchingEnabled() const noexcept { return _batched; }

    /**
     * Turns batching on or off for this form.
     *
     * By default, forms enable batching as a way to optimize performance. However, on certain
     * complex forms that contain multiple layers of overlapping text and transparent controls,
     * batching may cause some visual artifacts due alpha blending issues. In these cases,
     * turning batching off usually fixes the issue at a slight performance cost.
     *
     * @param enabled True to enable batching (default), false otherwise.
     */
    void setBatchingEnabled(bool enabled) { _batched = enabled; }

  private:
    /**
     * Constructor.
     */
    Form() = default;

    /**
     * Constructor.
     */
    Form(const Form& copy);

    /**
     * Destructor.
     */
    virtual ~Form();

    /**
     * @see Drawable::clone
     */
    Drawable* clone(NodeCloneContext& context);

    /**
     * @see Control::initialize
     */
    void initialize(const std::string& typeName, Theme::Style* style, Properties* properties);

    /**
     * Initialize a quad for this form in order to draw it in 3D.
     *
     * @param mesh The mesh to create a model from.
     */
    void initializeQuad(Mesh* mesh);

    /**
     * Updates all visible, enabled forms.
     */
    static void updateInternal(float elapsedTime);

    /**
     * Propagate touch events to enabled forms.
     *
     * @return Whether the touch event was consumed by a form.
     */
    static bool touchEventInternal(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

    /**
     * Propagate key events to enabled forms.
     *
     * @return Whether the key event was consumed by a form.
     */
    static bool keyEventInternal(Keyboard::KeyEvent evt, int key);

    /**
     * Propagate mouse events to enabled forms.
     *
     * @return True if the mouse event is consumed or false if it is not consumed.
     *
     * @see Mouse::MouseEvent
     */
    static bool mouseEventInternal(Mouse::MouseEvent evt, int x, int y, int wheelDelta);

    /**
     * Propagate gamepad button events to enabled forms.
     *
     * @see Control::gamepadButtonEventInternal
     */
    static bool gamepadButtonEventInternal(Gamepad* gamepad);

    /**
     * Propagate gamepad trigger events to enabled forms.
     *
     * @see Control::gamepadTriggerEventInternal
     */
    static bool gamepadTriggerEventInternal(Gamepad* gamepad, unsigned int index);

    /**
     * Propagate gamepad button events to enabled forms.
     *
     * @see Control::gamepadJoystickEventInternal
     */
    static bool gamepadJoystickEventInternal(Gamepad* gamepad, unsigned int index);

    /**
     * Fired by the platform when the game window resizes.
     *
     * @param width The new window width.
     * @param height The new window height.
     */
    static void resizeEventInternal(unsigned int width, unsigned int height);

    /**
     * Called during drawing to prepare a sprite batch for being drawn into for this form.
     */
    void startBatch(SpriteBatch* batch);

    /**
     * Called during drawing to signal completion of drawing into a batch.
     */
    void finishBatch(SpriteBatch* batch);

    /**
     * Unproject a point (from a mouse or touch event) into the scene and then project it onto the form.
     *
     * @param x The x coordinate of the mouse/touch point.
     * @param y The y coordinate of the mouse/touch point.
     * @param point A destination vector to populate with the projected point, in the form's plane.
     *
     * @return True if the projected point lies within the form's plane, false otherwise.
     */
    bool projectPoint(int x, int y, Vector3* point);

    const Matrix& getProjectionMatrix() const noexcept { return _projectionMatrix; }

    static bool pointerEventInternal(bool mouse, int evt, int x, int y, int param);

    static Control* findInputControl(int* x, int* y, bool focus, unsigned int contactIndex);

    static Control* findInputControl(Control* control,
                                     int x,
                                     int y,
                                     bool focus,
                                     unsigned int contactIndex);

    static Control* handlePointerPressRelease(int* x, int* y, bool pressed, unsigned int contactIndex);

    static Control* handlePointerMove(int* x, int* y, unsigned int contactIndex);

    static bool screenToForm(Control* ctrl, int* x, int* y);

    static void verifyRemovedControlState(Control* control);

    static void controlDisabled(Control* control);

    static void setFocusControl(Control* control);

    static void pollGamepads();

    static bool pollGamepad(Gamepad* gamepad);

    Matrix _projectionMatrix; // Projection matrix to be set on SpriteBatch objects when rendering the form
    std::vector<SpriteBatch*> _batches{};
    bool _batched{ true };
};

} // namespace tractor

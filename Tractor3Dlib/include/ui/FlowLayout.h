#pragma once

#include "ui/Layout.h"

namespace tractor
{

/**
 * Defines a flow layout.
 *
 * A flow layout arranges controls in order, left-to-right, row by row and wraps.
 */
class FlowLayout : public Layout
{
    friend class Form;
    friend class Container;

  public:
    /**
     * Get the type of this Layout.
     *
     * @return Layout::LAYOUT_FLOW
     */
    Layout::Type getType() const noexcept { return Layout::LAYOUT_FLOW; }

    /**
     * Returns the horizontal spacing between controls in the layout.
     *
     * @return The horizontal spacing between controls.
     */
    int getHorizontalSpacing() const noexcept { return _horizontalSpacing; }

    /**
     * Returns the vertical spacing between controls in the layout.
     *
     * @return The vertical spacing between controls.
     */
    int getVerticalSpacing() const noexcept { return _verticalSpacing; }

    /**
     * Sets the spacing to add between controls in the layout.
     *
     * @param horizontalSpacing The horizontal spacing between controls.
     * @param verticalSpacing The vertical spacing between controls.
     */
    void setSpacing(int horizontalSpacing, int verticalSpacing);

  protected:
    /**
     * Update the controls contained by the specified container.
     *
     * @param container The container to update.
     */
    void update(const Container* container);

    /**
     * Horizontal spacing between controls.
     */
    int _horizontalSpacing;

    /**
     * Vertical spacing between controls.
     */
    int _verticalSpacing;

  private:
    /**
     * Constructor.
     */
    FlowLayout();

    /**
     * Constructor.
     */
    FlowLayout(const FlowLayout& copy);

    /**
     * Destructor.
     */
    virtual ~FlowLayout();

    /**
     * Create a FlowLayout.
     *
     * @return A FlowLayout object.
     */
    static FlowLayout* create();
};

} // namespace tractor

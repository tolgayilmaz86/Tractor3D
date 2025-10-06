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

#include "ui/Layout.h"

namespace tractor
{

/**
 * Defines a Layout for forms and containers that requires the user
 * to specify absolute positions for all contained controls.
 */
class AbsoluteLayout : public Layout
{
    friend class Form;
    friend class Container;

  public:
    /**
     * Get the type of this Layout.
     *
     * @return Layout::LAYOUT_ABSOLUTE
     */
    Layout::Type getType() const noexcept;

  protected:
    /**
     * Update the controls contained by the specified container.
     *
     * An AbsoluteLayout does nothing to modify the layout of controls.
     * It simply calls update() on any control that is dirty.
     *
     * @param container The container to update.
     */
    void update(const Container* container);

  private:
    /*
     * Constructor.
     */
    AbsoluteLayout() = default;

    /*
     * Constructor.
     */
    AbsoluteLayout(const AbsoluteLayout& copy);

    /*
     * Destructor.
     */
    virtual ~AbsoluteLayout();

    /**
     * Create an AbsoluteLayout.
     *
     * @return An AbsoluteLayout object.
     */
    static AbsoluteLayout* create();
};

} // namespace tractor

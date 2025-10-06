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

#include "ui/FlowLayout.h"

#include "ui/Container.h"
#include "ui/Control.h"

namespace tractor
{

static FlowLayout* __instance;

FlowLayout::~FlowLayout() { __instance = nullptr; }

//----------------------------------------------------------------
FlowLayout* FlowLayout::create()
{
    if (!__instance)
        __instance = new FlowLayout();
    else
        __instance->addRef();

    return __instance;
}

//----------------------------------------------------------------
void FlowLayout::setSpacing(int horizontalSpacing, int verticalSpacing)
{
    _horizontalSpacing = horizontalSpacing;
    _verticalSpacing = verticalSpacing;
}

//----------------------------------------------------------------
void FlowLayout::update(const Container* container)
{
    assert(container);
    const Rectangle& containerBounds = container->getBounds();
    const Theme::Border& containerBorder = container->getBorder(container->getState());
    const Theme::Padding& containerPadding = container->getPadding();

    float clipWidth = containerBounds.width - containerBorder.left - containerBorder.right
                      - containerPadding.left - containerPadding.right;
    float clipHeight = containerBounds.height - containerBorder.top - containerBorder.bottom
                       - containerPadding.top - containerPadding.bottom;

    float xPosition = 0;
    float yPosition = 0;
    float rowY = 0;
    float tallestHeight = 0;

    std::vector<Control*> controls = container->getControls();
    for (size_t i = 0, controlsCount = controls.size(); i < controlsCount; i++)
    {
        Control* control = controls.at(i);
        assert(control);

        if (!control->isVisible()) continue;

        const Rectangle& bounds = control->getBounds();
        const Theme::Margin& margin = control->getMargin();

        xPosition += margin.left;

        // Wrap to next row if we've gone past the edge of the container.
        if (xPosition + bounds.width >= clipWidth)
        {
            xPosition = margin.left;
            rowY += tallestHeight + _verticalSpacing;
            tallestHeight = 0;
        }

        yPosition = rowY + margin.top;

        control->setPosition(xPosition, yPosition);

        xPosition += bounds.width + margin.right + _horizontalSpacing;

        float height = bounds.height + margin.top + margin.bottom;
        if (height > tallestHeight)
        {
            tallestHeight = height;
        }
    }
}

} // namespace tractor
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

#include "ui/VerticalLayout.h"

namespace tractor
{

VerticalLayout* VerticalLayout::create() { return new VerticalLayout(); }

void VerticalLayout::update(const Container* container)
{
    assert(container);

    // Need border, padding.
    Theme::Border border = container->getBorder(container->getState());
    Theme::Padding padding = container->getPadding();

    float yPosition = 0;

    const std::vector<Control*>& controls = container->getControls();

    int i, end, iter;
    if (_bottomToTop)
    {
        i = (int)controls.size() - 1;
        end = -1;
        iter = -1;
    }
    else
    {
        i = 0;
        end = (int)controls.size();
        iter = 1;
    }

    while (i != end)
    {
        Control* control = controls.at(i);
        assert(control);

        if (control->isVisible())
        {
            const Rectangle& bounds = control->getBounds();
            const Theme::Margin& margin = control->getMargin();

            yPosition += margin.top;

            control->setPosition(margin.left, yPosition);

            yPosition += bounds.height + margin.bottom + _spacing;
        }

        i += iter;
    }
}

} // namespace tractor
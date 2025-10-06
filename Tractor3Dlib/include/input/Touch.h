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

namespace tractor
{

/**
 * Defines a touch screen event.
 */
class Touch
{
  public:
    /**
     * Maximum simultaneous touch points supported.
     */
    static const unsigned int MAX_TOUCH_POINTS = 10;

    /**
     * The touch event type.
     */
    enum TouchEvent
    {
        TOUCH_PRESS,
        TOUCH_RELEASE,
        TOUCH_MOVE
    };

  private:
    /**
     * Constructor. Used internally.
     */
    Touch();
};

} // namespace tractor

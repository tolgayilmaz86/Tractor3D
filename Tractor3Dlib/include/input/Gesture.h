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
 * Defines a gesture touch screen event.
 */
class Gesture
{
  public:
    /**
     * The gesture event type.
     */
    enum GestureEvent
    {
        GESTURE_TAP = 0,
        GESTURE_SWIPE,
        GESTURE_PINCH,
        GESTURE_LONG_TAP,
        GESTURE_DRAG,
        GESTURE_DROP,
        GESTURE_ANY_SUPPORTED = -1,
    };

    /**
     * The up direction for a swipe event.
     */
    static const int SWIPE_DIRECTION_UP = 1 << 0;

    /**
     * The down direction for a swipe event.
     */
    static const int SWIPE_DIRECTION_DOWN = 1 << 1;

    /**
     * The left direction for a swipe event.
     */
    static const int SWIPE_DIRECTION_LEFT = 1 << 2;

    /**
     * The right direction for a swipe event.
     */
    static const int SWIPE_DIRECTION_RIGHT = 1 << 3;

  private:
    /**
     * Constructor. Used internally.
     */
    Gesture();
};

} // namespace tractor

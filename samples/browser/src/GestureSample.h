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

#include "Sample.h"
#include "tractor.h"

using namespace tractor;

/**
 * Samples gestures.
 */
class GestureSample : public Sample
{
  public:
    GestureSample() = default;

    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

    void gestureSwipeEvent(int x, int y, int direction);

    void gesturePinchEvent(int x, int y, float scale);

    void gestureTapEvent(int x, int y);

    void gestureLongTapEvent(int x, int y, float duration);

    void gestureDragEvent(int x, int y);

    void gestureDropEvent(int x, int y);

  protected:
    void initialize();

    void finalize();

    void update(float elapsedTime);

    void render(float elapsedTime);

  private:
    Font* _font{ nullptr };
    std::list<std::string> _eventLog{};
};

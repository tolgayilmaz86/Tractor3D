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

#include <functional>

#include "framework/Game.h"
#include "framework/Platform.h"

namespace tractor
{

/**
 * Defines a helper class for displaying images on screen for a duration of time.
 *
 * Ex. A splash or level loading screens.
 */
class ScreenDisplayer
{
  public:
    /**
     * Constructor.
     */
    ScreenDisplayer() = default;

    /**
     * Destructor.
     */
    ~ScreenDisplayer();

    /**
     * Displays a screen using the Game::renderOnce() mechanism for at least the given amount of time.
     *
     * @param instance See Game::renderOnce().
     * @param method See Game::renderOnce().
     * @param cookie See Game::renderOnce().
     * @param time The minimum amount of time to display the screen (in milliseconds).
     */
    template <typename T>
    void run(T* instance, std::function<void(void*)>, void* cookie, unsigned long time);

    /**
     * Starts a new screen displayer running; draws a screen using the {@link Game::renderOnce}
     * mechanism for at least the given amount of time.
     *
     * Note: this is intended for use from Lua scripts.
     *
     * @param function See {@link Game::renderOnce(const char*)}.
     * @param time The minimum amount of time to display the screen (in milliseconds).
     */
    static void start(const char* function, unsigned long time);

    /**
     * Finishes running the current screen displayer.
     *
     * Note: this is intended for use from Lua scripts.
     */
    static void finish();

  private:
    long _time{ 0L };
    double _startTime{ 0.0 };
    static ScreenDisplayer* __scriptInstance;
};

/**
 * Displays a screen using the {@link tractor::Game::renderOnce} mechanism for at least the given amount
 * of time. This function is intended to be called at the beginning of a block of code that is be
 * executed while the screen is displayed (i.e. Game#initialize). This function will block
 * at the end of the block of code in which it is called for the amount of time that has not yet elapsed.
 *
 * @param instance See {@link tractor::Game::renderOnce}.
 * @param method See {@link tractor::Game::renderOnce}.
 * @param cookie See {@link tractor::Game::renderOnce}.
 * @param time The minimum amount of time to display the screen (in milliseconds).
 */
template <typename T>
void ScreenDisplayer::run(T* instance, std::function<void(void*)> method, void* data, unsigned long time)
{
    _time = time;
    Game::getInstance()->renderOnce(instance, method, data);
    _startTime = Game::getInstance()->getGameTime();
}
} // namespace tractor

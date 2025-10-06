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
 * Defines a interface to be scheduled and called back at a later time using Game::schedule().
 *
 * @script{ignore}
 */
class TimeListener
{
  public:
    /**
     * Callback method that is called when the scheduled event is fired.
     *
     * @param timeDiff The time difference between the current game time and the target time.
     *                 The time differences will always be non-negative because scheduled events
     * will not fire early.
     * @param cookie The cookie data that was passed when the event was scheduled.
     */
    virtual void timeEvent(long timeDiff, void* cookie) = 0;
};

} // namespace tractor

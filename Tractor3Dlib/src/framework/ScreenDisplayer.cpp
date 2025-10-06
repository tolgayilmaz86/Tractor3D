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

#include "framework/ScreenDisplayer.h"

namespace tractor
{

ScreenDisplayer* ScreenDisplayer::__scriptInstance = nullptr;

ScreenDisplayer::ScreenDisplayer() : _time(0L), _startTime(0) {}

ScreenDisplayer::~ScreenDisplayer()
{
    long elapsedTime = (long)(Game::getInstance()->getGameTime() - _startTime);
    if (elapsedTime < _time) Platform::sleep(_time - elapsedTime);
}

void ScreenDisplayer::start(const char* function, unsigned long time)
{
    if (__scriptInstance == nullptr)
    {
        __scriptInstance = new ScreenDisplayer();
    }

    __scriptInstance->_time = time;
    Game::getInstance()->renderOnce(function);
    __scriptInstance->_startTime = Game::getInstance()->getGameTime();
}

void ScreenDisplayer::finish() { SAFE_DELETE(__scriptInstance); }

} // namespace tractor

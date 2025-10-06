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

#include <windows.h>

#include <chrono>
#include <iostream>

namespace tractor
{
class Profiler
{
  public:
    Profiler(const std::string& name)
        : _name(name), _start(std::chrono::high_resolution_clock::now())
    {
        if (AllocConsole())
        {
            // Redirect stdout to the console
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
        }
    }

    ~Profiler()
    {
        auto end = std::chrono::high_resolution_clock::now();
        float elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - _start).count();
        std::cout << "Time taken for " << _name << ": " << elapsed / 1000.f << " milliseconds"
                  << std::endl;

        FreeConsole();
    }

  private:
    std::string _name;
    std::chrono::high_resolution_clock::time_point _start;
};
} // namespace tractor

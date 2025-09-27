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

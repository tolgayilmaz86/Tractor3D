#include "pch.h"

#include "math/MathUtil.h"

namespace tractor
{

void MathUtil::smooth(float* x, float target, float elapsedTime, float responseTime)
{
    assert(x);

    if (elapsedTime > 0)
    {
        *x += (target - *x) * elapsedTime / (elapsedTime + responseTime);
    }
}

void MathUtil::smooth(float* x, float target, float elapsedTime, float riseTime, float fallTime)
{
    assert(x);

    if (elapsedTime > 0)
    {
        float delta = target - *x;
        *x += delta * elapsedTime / (elapsedTime + (delta > 0 ? riseTime : fallTime));
    }
}

} // namespace tractor

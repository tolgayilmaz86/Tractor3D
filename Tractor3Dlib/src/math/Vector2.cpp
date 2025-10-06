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

#include "math/Vector2.h"

namespace tractor
{

Vector2::Vector2() : x(0.0f), y(0.0f) {}

Vector2::Vector2(float x, float y) : x(x), y(y) {}

Vector2::Vector2(const float* array) { set(array); }

Vector2::Vector2(const Vector2& p1, const Vector2& p2) { set(p1, p2); }

Vector2::Vector2(const Vector2& copy) { set(copy); }

const Vector2& Vector2::zero()
{
    static Vector2 value(0.0f, 0.0f);
    return value;
}

const Vector2& Vector2::one()
{
    static Vector2 value(1.0f, 1.0f);
    return value;
}

const Vector2& Vector2::unitX()
{
    static Vector2 value(1.0f, 0.0f);
    return value;
}

const Vector2& Vector2::unitY()
{
    static Vector2 value(0.0f, 1.0f);
    return value;
}

float Vector2::angle(const Vector2& v1, const Vector2& v2)
{
    float dz = v1.x * v2.y - v1.y * v2.x;
    return atan2f(fabsf(dz) + MATH_FLOAT_SMALL, dot(v1, v2));
}

void Vector2::add(const Vector2& v)
{
    x += v.x;
    y += v.y;
}

void Vector2::add(const Vector2& v1, const Vector2& v2, Vector2* dst)
{
    assert(dst);

    dst->x = v1.x + v2.x;
    dst->y = v1.y + v2.y;
}

void Vector2::clamp(const Vector2& min, const Vector2& max)
{
    assert(!(min.x > max.x || min.y > max.y));

    // Clamp the x, y values.
    x = std::clamp(x, min.x, max.x);
    y = std::clamp(y, min.y, max.y);
}

void Vector2::clamp(const Vector2& v, const Vector2& min, const Vector2& max, Vector2* dst)
{
    assert(dst);
    assert(!(min.x > max.x || min.y > max.y));

    // Clamp the x, y values.
    dst->x = std::clamp(v.x, min.x, max.x);
    dst->y = std::clamp(v.y, min.y, max.y);
}

float Vector2::distance(const Vector2& v) const
{
    float dx = v.x - x;
    float dy = v.y - y;

    return sqrt(dx * dx + dy * dy);
}

float Vector2::distanceSquared(const Vector2& v) const
{
    float dx = v.x - x;
    float dy = v.y - y;
    return (dx * dx + dy * dy);
}

float Vector2::dot(const Vector2& v) const noexcept { return (x * v.x + y * v.y); }

float Vector2::dot(const Vector2& v1, const Vector2& v2) noexcept
{
    return (v1.x * v2.x + v1.y * v2.y);
}

float Vector2::length() const { return sqrt(x * x + y * y); }

float Vector2::lengthSquared() const noexcept { return (x * x + y * y); }

void Vector2::negate() noexcept
{
    x = -x;
    y = -y;
}

Vector2& Vector2::normalize()
{
    normalize(this);
    return *this;
}

void Vector2::normalize(Vector2* dst) const
{
    assert(dst);

    if (dst != this)
    {
        dst->x = x;
        dst->y = y;
    }

    float n = x * x + y * y;
    // Already normalized.
    if (n == 1.0f) return;

    n = sqrt(n);
    // Too close to zero.
    if (n < MATH_TOLERANCE) return;

    n = 1.0f / n;
    dst->x *= n;
    dst->y *= n;
}

void Vector2::scale(float scalar)
{
    x *= scalar;
    y *= scalar;
}

void Vector2::scale(const Vector2& scale)
{
    x *= scale.x;
    y *= scale.y;
}

void Vector2::rotate(const Vector2& point, float angle)
{
    double sinAngle = sin(angle);
    double cosAngle = cos(angle);

    if (point.isZero())
    {
        float tempX = x * cosAngle - y * sinAngle;
        y = y * cosAngle + x * sinAngle;
        x = tempX;
    }
    else
    {
        float tempX = x - point.x;
        float tempY = y - point.y;

        x = tempX * cosAngle - tempY * sinAngle + point.x;
        y = tempY * cosAngle + tempX * sinAngle + point.y;
    }
}

void Vector2::set(float x, float y)
{
    this->x = x;
    this->y = y;
}

void Vector2::set(const float* array)
{
    assert(array);

    x = array[0];
    y = array[1];
}

void Vector2::set(const Vector2& v)
{
    this->x = v.x;
    this->y = v.y;
}

void Vector2::set(const Vector2& p1, const Vector2& p2)
{
    x = p2.x - p1.x;
    y = p2.y - p1.y;
}

void Vector2::subtract(const Vector2& v)
{
    x -= v.x;
    y -= v.y;
}

void Vector2::subtract(const Vector2& v1, const Vector2& v2, Vector2* dst)
{
    assert(dst);

    dst->x = v1.x - v2.x;
    dst->y = v1.y - v2.y;
}

void Vector2::smooth(const Vector2& target, float elapsedTime, float responseTime)
{
    if (elapsedTime > 0)
    {
        *this += (target - *this) * (elapsedTime / (elapsedTime + responseTime));
    }
}

} // namespace tractor

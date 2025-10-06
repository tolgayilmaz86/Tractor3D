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

#include "math/Vector4.h"

namespace tractor
{

//-----------------------------------------------------------------------------
Vector4::Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

//-----------------------------------------------------------------------------
Vector4::Vector4(const float* src) { set(src); }

//-----------------------------------------------------------------------------
Vector4::Vector4(const Vector4& p1, const Vector4& p2) { set(p1, p2); }

//-----------------------------------------------------------------------------
Vector4::Vector4(const Vector4& copy) { set(copy); }

//-----------------------------------------------------------------------------
Vector4 Vector4::fromColor(unsigned int color)
{
    float components[4];
    int componentIndex = 0;
    for (int i = 3; i >= 0; --i)
    {
        int component = (color >> i * 8) & 0x000000ff;

        components[componentIndex++] = static_cast<float>(component) / 255.0f;
    }

    Vector4 value(components);
    return value;
}

//-----------------------------------------------------------------------------
const Vector4& Vector4::zero()
{
    static Vector4 value(0.0f, 0.0f, 0.0f, 0.0f);
    return value;
}

//-----------------------------------------------------------------------------
const Vector4& Vector4::one()
{
    static Vector4 value(1.0f, 1.0f, 1.0f, 1.0f);
    return value;
}

//-----------------------------------------------------------------------------
const Vector4& Vector4::unitX()
{
    static Vector4 value(1.0f, 0.0f, 0.0f, 0.0f);
    return value;
}

//-----------------------------------------------------------------------------
const Vector4& Vector4::unitY()
{
    static Vector4 value(0.0f, 1.0f, 0.0f, 0.0f);
    return value;
}

//-----------------------------------------------------------------------------
const Vector4& Vector4::unitZ()
{
    static Vector4 value(0.0f, 0.0f, 1.0f, 0.0f);
    return value;
}

//-----------------------------------------------------------------------------
const Vector4& Vector4::unitW()
{
    static Vector4 value(0.0f, 0.0f, 0.0f, 1.0f);
    return value;
}

//-----------------------------------------------------------------------------
float Vector4::angle(const Vector4& v1, const Vector4& v2)
{
    float dx = v1.w * v2.x - v1.x * v2.w - v1.y * v2.z + v1.z * v2.y;
    float dy = v1.w * v2.y - v1.y * v2.w - v1.z * v2.x + v1.x * v2.z;
    float dz = v1.w * v2.z - v1.z * v2.w - v1.x * v2.y + v1.y * v2.x;

    return atan2f(sqrt(dx * dx + dy * dy + dz * dz) + MATH_FLOAT_SMALL, dot(v1, v2));
}

//-----------------------------------------------------------------------------
void Vector4::add(const Vector4& v)
{
    x += v.x;
    y += v.y;
    z += v.z;
    w += v.w;
}

//-----------------------------------------------------------------------------
void Vector4::add(const Vector4& v1, const Vector4& v2, Vector4* dst)
{
    assert(dst);

    dst->x = v1.x + v2.x;
    dst->y = v1.y + v2.y;
    dst->z = v1.z + v2.z;
    dst->w = v1.w + v2.w;
}

//-----------------------------------------------------------------------------
void Vector4::clamp(const Vector4& min, const Vector4& max)
{
    assert(!(min.x > max.x || min.y > max.y || min.z > max.z || min.w > max.w));

    // Clamp the x, y, z and w values.
    x = std::clamp(x, min.x, max.x);
    y = std::clamp(y, min.y, max.y);
    z = std::clamp(z, min.z, max.z);
    w = std::clamp(w, min.w, max.w);
}

//-----------------------------------------------------------------------------
void Vector4::clamp(const Vector4& v, const Vector4& min, const Vector4& max, Vector4* dst)
{
    assert(dst);
    assert(!(min.x > max.x || min.y > max.y || min.z > max.z || min.w > max.w));

    // Clamp the x, y, z and w values.
    dst->x = std::clamp(v.x, min.x, max.x);
    dst->y = std::clamp(v.y, min.y, max.y);
    dst->z = std::clamp(v.z, min.z, max.z);
    dst->w = std::clamp(v.w, min.w, max.w);
}

//-----------------------------------------------------------------------------
float Vector4::distance(const Vector4& v) const
{
    float dx = v.x - x;
    float dy = v.y - y;
    float dz = v.z - z;
    float dw = v.w - w;

    return sqrt(dx * dx + dy * dy + dz * dz + dw * dw);
}

//-----------------------------------------------------------------------------
float Vector4::distanceSquared(const Vector4& v) const
{
    float dx = v.x - x;
    float dy = v.y - y;
    float dz = v.z - z;
    float dw = v.w - w;

    return (dx * dx + dy * dy + dz * dz + dw * dw);
}

//-----------------------------------------------------------------------------
void Vector4::negate()
{
    x = -x;
    y = -y;
    z = -z;
    w = -w;
}

//-----------------------------------------------------------------------------
Vector4& Vector4::normalize()
{
    normalize(this);
    return *this;
}

//-----------------------------------------------------------------------------
void Vector4::normalize(Vector4* dst) const
{
    assert(dst);

    if (dst != this)
    {
        dst->x = x;
        dst->y = y;
        dst->z = z;
        dst->w = w;
    }

    float n = x * x + y * y + z * z + w * w;
    // Already normalized.
    if (n == 1.0f) return;

    n = sqrt(n);
    // Too close to zero.
    if (n < MATH_TOLERANCE) return;

    n = 1.0f / n;
    dst->x *= n;
    dst->y *= n;
    dst->z *= n;
    dst->w *= n;
}

//-----------------------------------------------------------------------------
void Vector4::scale(float scalar)
{
    x *= scalar;
    y *= scalar;
    z *= scalar;
    w *= scalar;
}

//-----------------------------------------------------------------------------
void Vector4::set(float x, float y, float z, float w)
{
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
}

//-----------------------------------------------------------------------------
void Vector4::set(const float* array)
{
    assert(array);

    x = array[0];
    y = array[1];
    z = array[2];
    w = array[3];
}

//-----------------------------------------------------------------------------
void Vector4::set(const Vector4& v)
{
    this->x = v.x;
    this->y = v.y;
    this->z = v.z;
    this->w = v.w;
}

//-----------------------------------------------------------------------------
void Vector4::set(const Vector4& p1, const Vector4& p2)
{
    x = p2.x - p1.x;
    y = p2.y - p1.y;
    z = p2.z - p1.z;
    w = p2.w - p1.w;
}

//-----------------------------------------------------------------------------
void Vector4::subtract(const Vector4& v)
{
    x -= v.x;
    y -= v.y;
    z -= v.z;
    w -= v.w;
}

//-----------------------------------------------------------------------------
void Vector4::subtract(const Vector4& v1, const Vector4& v2, Vector4* dst)
{
    assert(dst);

    dst->x = v1.x - v2.x;
    dst->y = v1.y - v2.y;
    dst->z = v1.z - v2.z;
    dst->w = v1.w - v2.w;
}

} // namespace tractor

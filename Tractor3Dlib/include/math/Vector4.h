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

#include "math/VectorBase.h"

namespace tractor
{

class Matrix;

/**
 * Defines 4-element floating point vector.
 */
class Vector4 : public VectorBase<Vector4, 4>
{
    friend class VectorBase<Vector4, 4>;
  public:
    /**
     * The x-coordinate.
     */
    float x{ 0.0f };

    /**
     * The y-coordinate.
     */
    float y{ 0.0f };

    /**
     * The z-coordinate.
     */
    float z{ 0.0f };

    /**
     * The w-coordinate.
     */
    float w{ 0.0f };

    /**
     * Constructs a new vector initialized to all zeros.
     */
    Vector4() = default;

    /**
     * Constructs a new vector initialized to the specified values.
     *
     * @param x The x coordinate.
     * @param y The y coordinate.
     * @param z The z coordinate.
     * @param w The w coordinate.
     */
    Vector4(float x, float y, float z, float w);

    /**
     * Constructs a new vector from the values in the specified array.
     *
     * @param array An array containing the elements of the vector in the order x, y, z, w.
     */
    Vector4(const float* array);

    /**
     * Constructs a vector that describes the direction between the specified points.
     *
     * @param p1 The first point.
     * @param p2 The second point.
     */
    Vector4(const Vector4& p1, const Vector4& p2);

    /**
     * Constructor.
     *
     * Creates a new vector that is a copy of the specified vector.
     *
     * @param copy The vector to copy.
     */
    Vector4(const Vector4& copy);

    /**
     * Creates a new vector from an integer interpreted as an RGBA value.
     * E.g. 0xff0000ff represents opaque red or the vector (1, 0, 0, 1).
     *
     * @param color The integer to interpret as an RGBA value.
     *
     * @return A vector corresponding to the interpreted RGBA color.
     */
    static Vector4 fromColor(unsigned int color);

    /**
     * Destructor.
     */
    ~Vector4() = default;

    /**
     * Returns the zero vector.
     *
     * @return The 4-element vector of 0s.
     */
    static const Vector4& zero();

    /**
     * Returns the one vector.
     *
     * @return The 4-element vector of 1s.
     */
    static const Vector4& one();

    /**
     * Returns the unit x vector.
     *
     * @return The 4-element unit vector along the x axis.
     */
    static const Vector4& unitX();

    /**
     * Returns the unit y vector.
     *
     * @return The 4-element unit vector along the y axis.
     */
    static const Vector4& unitY();

    /**
     * Returns the unit z vector.
     *
     * @return The 4-element unit vector along the z axis.
     */
    static const Vector4& unitZ();

    /**
     * Returns the unit w vector.
     *
     * @return The 4-element unit vector along the w axis.
     */
    static const Vector4& unitW();

    /**
     * Indicates whether this vector contains all zeros.
     *
     * @return true if this vector contains all zeros, false otherwise.
     */
    bool isZero() const noexcept { return x == 0.0f && y == 0.0f && z == 0.0f && w == 0.0f; }

    /**
     * Indicates whether this vector contains all ones.
     *
     * @return true if this vector contains all ones, false otherwise.
     */
    bool isOne() const noexcept { return x == 1.0f && y == 1.0f && z == 1.0f && w == 1.0f; }

    /**
     * Returns the angle (in radians) between the specified vectors.
     *
     * @param v1 The first vector.
     * @param v2 The second vector.
     *
     * @return The angle between the two vectors (in radians).
     */
    static float angle(const Vector4& v1, const Vector4& v2);

    // Note: add(const Vector4& v) is inherited from VectorBase
    using VectorBase::add;

    /**
     * Adds the specified vectors and stores the result in dst.
     *
     * @param v1 The first vector.
     * @param v2 The second vector.
     * @param dst A vector to store the result in.
     */
    static void add(const Vector4& v1, const Vector4& v2, Vector4* dst);

    /**
     * Clamps this vector within the specified range.
     *
     * @param min The minimum value.
     * @param max The maximum value.
     */
    void clamp(const Vector4& min, const Vector4& max);

    /**
     * Clamps the specified vector within the specified range and returns it in dst.
     *
     * @param v The vector to clamp.
     * @param min The minimum value.
     * @param max The maximum value.
     * @param dst A vector to store the result in.
     */
    static void clamp(const Vector4& v, const Vector4& min, const Vector4& max, Vector4* dst);

    // Note: distance, distanceSquared, dot, length, lengthSquared are inherited from VectorBase
    using VectorBase::distance;
    using VectorBase::distanceSquared;
    using VectorBase::dot;
    using VectorBase::length;
    using VectorBase::lengthSquared;

    /**
     * Returns the dot product between the specified vectors.
     *
     * @param v1 The first vector.
     * @param v2 The second vector.
     *
     * @return The dot product between the vectors.
     */
    static float dot(const Vector4& v1, const Vector4& v2)
    {
        return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w);
    }

    // Note: negate, normalize, scale are inherited from VectorBase
    using VectorBase::negate;
    using VectorBase::normalize;
    using VectorBase::scale;

    /**
     * Sets the elements of this vector to the specified values.
     *
     * @param x The new x coordinate.
     * @param y The new y coordinate.
     * @param z The new z coordinate.
     * @param w The new w coordinate.
     */
    void set(float x, float y, float z, float w);

    /**
     * Sets the elements of this vector from the values in the specified array.
     *
     * @param array An array containing the elements of the vector in the order x, y, z, w.
     */
    void set(const float* array);

    /**
     * Sets the elements of this vector to those in the specified vector.
     *
     * @param v The vector to copy.
     */
    void set(const Vector4& v);

    /**
     * Sets this vector to the directional vector between the specified points.
     *
     * @param p1 The first point.
     * @param p2 The second point.
     */
    void set(const Vector4& p1, const Vector4& p2);

    // Note: subtract(const Vector4& v) is inherited from VectorBase
    using VectorBase::subtract;

    /**
     * Subtracts the specified vectors and stores the result in dst.
     * The resulting vector is computed as (v1 - v2).
     *
     * @param v1 The first vector.
     * @param v2 The second vector.
     * @param dst The destination vector.
     */
    static void subtract(const Vector4& v1, const Vector4& v2, Vector4* dst);

    // Note: Operators +, +=, -, -=, unary-, *, *=, /, ==, != are inherited from VectorBase
    using VectorBase::operator+;
    using VectorBase::operator+=;
    using VectorBase::operator-;
    using VectorBase::operator-=;
    using VectorBase::operator*;
    using VectorBase::operator*=;
    using VectorBase::operator/;
    using VectorBase::operator==;
    using VectorBase::operator!=;

    /**
     * Determines if this vector is less than the given vector.
     *
     * @param v The vector to compare against.
     *
     * @return True if this vector is less than the given vector, false otherwise.
     */
    bool operator<(const Vector4& v) const
    {
        if (x == v.x)
        {
            if (y == v.y)
            {
                if (z == v.z)
                {
                    return w < v.w;
                }
                return z < v.z;
            }
            return y < v.y;
        }
        return x < v.x;
    }
};

// Note: operator*(float, Vector4) is provided by VectorBase template

} // namespace tractor

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
 * Defines a 2-element floating point vector.
 */
class Vector2 : public VectorBase<Vector2, 2>
{
    friend class VectorBase<Vector2, 2>;
  public:
    /**
     * The x coordinate.
     */
    float x{ 0.0f };

    /**
     * The y coordinate.
     */
    float y{ 0.0f };

    /**
     * Constructs a new vector initialized to all zeros.
     */
    Vector2() = default;

    /**
     * Constructs a new vector initialized to the specified values.
     *
     * @param x The x coordinate.
     * @param y The y coordinate.
     */
    Vector2(float x, float y);

    /**
     * Constructs a new vector from the values in the specified array.
     *
     * @param array An array containing the elements of the vector in the order x, y.
     */
    Vector2(const float* array);

    /**
     * Constructs a vector that describes the direction between the specified points.
     *
     * @param p1 The first point.
     * @param p2 The second point.
     */
    Vector2(const Vector2& p1, const Vector2& p2);

    /**
     * Constructs a new vector that is a copy of the specified vector.
     *
     * @param copy The vector to copy.
     */
    Vector2(const Vector2& copy);

    /**
     * Destructor.
     */
    ~Vector2() = default;

    /**
     * Returns the zero vector.
     *
     * @return The 2-element vector of 0s.
     */
    static const Vector2& zero();

    /**
     * Returns the one vector.
     *
     * @return The 2-element vector of 1s.
     */
    static const Vector2& one();

    /**
     * Returns the unit x vector.
     *
     * @return The 2-element unit vector along the x axis.
     */
    static const Vector2& unitX();

    /**
     * Returns the unit y vector.
     *
     * @return The 2-element unit vector along the y axis.
     */
    static const Vector2& unitY();

    /**
     * Indicates whether this vector contains all zeros.
     *
     * @return true if this vector contains all zeros, false otherwise.
     */
    bool isZero() const noexcept { return x == 0.0f && y == 0.0f; }

    /**
     * Indicates whether this vector contains all ones.
     *
     * @return true if this vector contains all ones, false otherwise.
     */
    bool isOne() const noexcept { return x == 1.0f && y == 1.0f; }

    /**
     * Returns the angle (in radians) between the specified vectors.
     *
     * @param v1 The first vector.
     * @param v2 The second vector.
     *
     * @return The angle between the two vectors (in radians).
     */
    static float angle(const Vector2& v1, const Vector2& v2);

    // Note: add(const Vector2& v) is inherited from VectorBase
    using VectorBase::add;

    /**
     * Adds the specified vectors and stores the result in dst.
     *
     * @param v1 The first vector.
     * @param v2 The second vector.
     * @param dst A vector to store the result in.
     */
    static void add(const Vector2& v1, const Vector2& v2, Vector2* dst);

    /**
     * Clamps this vector within the specified range.
     *
     * @param min The minimum value.
     * @param max The maximum value.
     */
    void clamp(const Vector2& min, const Vector2& max);

    /**
     * Clamps the specified vector within the specified range and returns it in dst.
     *
     * @param v The vector to clamp.
     * @param min The minimum value.
     * @param max The maximum value.
     * @param dst A vector to store the result in.
     */
    static void clamp(const Vector2& v, const Vector2& min, const Vector2& max, Vector2* dst);

    // Note: distance, distanceSquared, dot, length, lengthSquared, negate, normalize, scale(float)
    // are inherited from VectorBase
    using VectorBase::distance;
    using VectorBase::distanceSquared;
    using VectorBase::dot;
    using VectorBase::length;
    using VectorBase::lengthSquared;
    using VectorBase::negate;
    using VectorBase::normalize;
    using VectorBase::scale;

    /**
     * Returns the dot product between the specified vectors.
     *
     * @param v1 The first vector.
     * @param v2 The second vector.
     *
     * @return The dot product between the vectors.
     */
    static float dot(const Vector2& v1, const Vector2& v2) noexcept;

    /**
     * Scales each element of this vector by the matching component of scale.
     *
     * @param scale The vector to scale by.
     */
    void scale(const Vector2& scale);

    /**
     * Rotates this vector by angle (specified in radians) around the given point.
     *
     * @param point The point to rotate around.
     * @param angle The angle to rotate by (in radians).
     */
    void rotate(const Vector2& point, float angle);

    /**
     * Sets the elements of this vector to the specified values.
     *
     * @param x The new x coordinate.
     * @param y The new y coordinate.
     */
    void set(float x, float y);

    /**
     * Sets the elements of this vector from the values in the specified array.
     *
     * @param array An array containing the elements of the vector in the order x, y.
     */
    void set(const float* array);

    /**
     * Sets the elements of this vector to those in the specified vector.
     *
     * @param v The vector to copy.
     */
    void set(const Vector2& v);

    /**
     * Sets this vector to the directional vector between the specified points.
     *
     * @param p1 The first point.
     * @param p2 The second point.
     */
    void set(const Vector2& p1, const Vector2& p2);

    // Note: subtract(const Vector2& v) and smooth are inherited from VectorBase
    using VectorBase::subtract;
    using VectorBase::smooth;

    /**
     * Subtracts the specified vectors and stores the result in dst.
     * The resulting vector is computed as (v1 - v2).
     *
     * @param v1 The first vector.
     * @param v2 The second vector.
     * @param dst The destination vector.
     */
    static void subtract(const Vector2& v1, const Vector2& v2, Vector2* dst);

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
    bool operator<(const Vector2& v) const
    {
        if (x == v.x)
        {
            return y < v.y;
        }
        return x < v.x;
    }

};

// Note: operator*(float, Vector2) is provided by VectorBase template

} // namespace tractor

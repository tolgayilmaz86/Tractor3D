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

/**
 * Defines a 3-element floating point vector.
 *
 * When using a vector to represent a surface normal,
 * the vector should typically be normalized.
 * Other uses of directional vectors may wish to leave
 * the magnitude of the vector intact. When used as a point,
 * the elements of the vector represent a position in 3D space.
 */
class Vector3 : public VectorBase<Vector3, 3>
{
    friend class VectorBase<Vector3, 3>;
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
     * Constructs a new vector initialized to all zeros.
     */
    Vector3() = default;

    /**
     * Constructs a new vector initialized to the specified values.
     *
     * @param x The x coordinate.
     * @param y The y coordinate.
     * @param z The z coordinate.
     */
    Vector3(float x, float y, float z);

    /**
     * Constructs a new vector from the values in the specified array.
     *
     * @param array An array containing the elements of the vector in the order x, y, z.
     */
    Vector3(const float* array);

    /**
     * Constructs a vector that describes the direction between the specified points.
     *
     * @param p1 The first point.
     * @param p2 The second point.
     */
    Vector3(const Vector3& p1, const Vector3& p2);

    /**
     * Constructs a new vector that is a copy of the specified vector.
     *
     * @param copy The vector to copy.
     */
    Vector3(const Vector3& copy);

    /**
     * Creates a new vector from an integer interpreted as an RGB value.
     * E.g. 0xff0000 represents red or the vector (1, 0, 0).
     *
     * @param color The integer to interpret as an RGB value.
     *
     * @return A vector corresponding to the interpreted RGB color.
     */
    static Vector3 fromColor(unsigned int color);

    /**
     * Destructor.
     */
    ~Vector3() = default;

    /**
     * Returns the zero vector.
     *
     * @return The 3-element vector of 0s.
     */
    static const Vector3& zero();

    /**
     * Returns the one vector.
     *
     * @return The 3-element vector of 1s.
     */
    static const Vector3& one();

    /**
     * Returns the unit x vector.
     *
     * @return The 3-element unit vector along the x axis.
     */
    static const Vector3& unitX();

    /**
     * Returns the unit y vector.
     *
     * @return The 3-element unit vector along the y axis.
     */
    static const Vector3& unitY();

    /**
     * Returns the unit z vector.
     *
     * @return The 3-element unit vector along the z axis.
     */
    static const Vector3& unitZ();

    /**
     * Indicates whether this vector contains all zeros.
     *
     * @return true if this vector contains all zeros, false otherwise.
     */
    bool isZero() const { return x == 0.0f && y == 0.0f && z == 0.0f; }

    /**
     * Indicates whether this vector contains all ones.
     *
     * @return true if this vector contains all ones, false otherwise.
     */
    bool isOne() const { return x == 1.0f && y == 1.0f && z == 1.0f; }

    /**
     * Returns the angle (in radians) between the specified vectors.
     *
     * @param v1 The first vector.
     * @param v2 The second vector.
     *
     * @return The angle between the two vectors (in radians).
     */
    static float angle(const Vector3& v1, const Vector3& v2);

    // Note: add(const Vector3& v) is inherited from VectorBase
    using VectorBase::add;

    /**
     * Adds the specified vectors and stores the result in dst.
     *
     * @param v1 The first vector.
     * @param v2 The second vector.
     * @param dst A vector to store the result in.
     */
    static void add(const Vector3& v1, const Vector3& v2, Vector3* dst);

    /**
     * Clamps this vector within the specified range.
     *
     * @param min The minimum value.
     * @param max The maximum value.
     */
    void clamp(const Vector3& min, const Vector3& max);

    /**
     * Clamps the specified vector within the specified range and returns it in dst.
     *
     * @param v The vector to clamp.
     * @param min The minimum value.
     * @param max The maximum value.
     * @param dst A vector to store the result in.
     */
    static void clamp(const Vector3& v, const Vector3& min, const Vector3& max, Vector3* dst);

    /**
     * Sets this vector to the cross product between itself and the specified vector.
     *
     * @param v The vector to compute the cross product with.
     */
    void cross(const Vector3& v) { cross(*this, v, this); }

    /**
     * Computes the cross product of the specified vectors and stores the result in dst.
     *
     * @param v1 The first vector.
     * @param v2 The second vector.
     * @param dst A vector to store the result in.
     */
    static void cross(const Vector3& v1, const Vector3& v2, Vector3* dst);

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
    static float dot(const Vector3& v1, const Vector3& v2);

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
     */
    void set(float x, float y, float z);

    /**
     * Sets the elements of this vector from the values in the specified array.
     *
     * @param array An array containing the elements of the vector in the order x, y, z.
     */
    void set(const float* array);

    /**
     * Sets the elements of this vector to those in the specified vector.
     *
     * @param v The vector to copy.
     */
    void set(const Vector3& v);

    /**
     * Sets this vector to the directional vector between the specified points.
     */
    void set(const Vector3& p1, const Vector3& p2);

    // Note: subtract(const Vector3& v) and smooth are inherited from VectorBase
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
    static void subtract(const Vector3& v1, const Vector3& v2, Vector3* dst);

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
    bool operator<(const Vector3& v) const
    {
        if (x == v.x)
        {
            if (y == v.y)
            {
                return z < v.z;
            }
            return y < v.y;
        }
        return x < v.x;
    }
};

// Note: operator*(float, Vector3) is provided by VectorBase template

} // namespace tractor

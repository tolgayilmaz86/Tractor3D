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

#include <cassert>
#include <cmath>

namespace tractor
{

// Forward declaration for MATH_TOLERANCE
#ifndef MATH_TOLERANCE
#define MATH_TOLERANCE 2e-37f
#endif

/**
 * CRTP base class for N-dimensional floating point vectors.
 *
 * This template provides common operations shared across Vector2, Vector3, and Vector4,
 * eliminating code duplication while maintaining zero-overhead abstraction through
 * static polymorphism.
 *
 * @tparam Derived The derived vector class (Vector2, Vector3, or Vector4).
 * @tparam N The number of components in the vector.
 */
template <typename Derived, size_t N>
class VectorBase
{
  public:
    /**
     * Returns a pointer to the underlying data array.
     * Derived classes must have contiguous float members starting with x.
     */
    float* data() noexcept { return &(static_cast<Derived*>(this)->x); }
    const float* data() const noexcept { return &(static_cast<const Derived*>(this)->x); }

    /**
     * Returns the number of components in this vector.
     */
    static constexpr size_t size() noexcept { return N; }

    /**
     * Computes the squared length of this vector.
     * Prefer this over length() when comparing magnitudes.
     *
     * @return The squared length of the vector.
     */
    float lengthSquared() const noexcept
    {
        const float* d = data();
        float sum = 0.0f;
        for (size_t i = 0; i < N; ++i)
            sum += d[i] * d[i];
        return sum;
    }

    /**
     * Computes the length of this vector.
     *
     * @return The length of the vector.
     */
    float length() const { return std::sqrt(lengthSquared()); }

    /**
     * Returns the distance between this vector and v.
     *
     * @param v The other vector.
     * @return The distance between this vector and v.
     */
    float distance(const Derived& v) const { return std::sqrt(distanceSquared(v)); }

    /**
     * Returns the squared distance between this vector and v.
     * Prefer this over distance() when comparing distances.
     *
     * @param v The other vector.
     * @return The squared distance between this vector and v.
     */
    float distanceSquared(const Derived& v) const
    {
        const float* d1 = data();
        const float* d2 = v.data();
        float sum = 0.0f;
        for (size_t i = 0; i < N; ++i)
        {
            float diff = d2[i] - d1[i];
            sum += diff * diff;
        }
        return sum;
    }

    /**
     * Returns the dot product of this vector and the specified vector.
     *
     * @param v The vector to compute the dot product with.
     * @return The dot product.
     */
    float dot(const Derived& v) const noexcept
    {
        const float* d1 = data();
        const float* d2 = v.data();
        float sum = 0.0f;
        for (size_t i = 0; i < N; ++i)
            sum += d1[i] * d2[i];
        return sum;
    }

    /**
     * Negates this vector in place.
     */
    void negate() noexcept
    {
        float* d = data();
        for (size_t i = 0; i < N; ++i)
            d[i] = -d[i];
    }

    /**
     * Normalizes this vector to unit length.
     * If the vector is zero or already normalized, no change is made.
     *
     * @return Reference to this vector after normalization.
     */
    Derived& normalize()
    {
        float n = lengthSquared();
        if (n == 1.0f)
            return self();

        n = std::sqrt(n);
        if (n < MATH_TOLERANCE)
            return self();

        float invN = 1.0f / n;
        float* d = data();
        for (size_t i = 0; i < N; ++i)
            d[i] *= invN;

        return self();
    }

    /**
     * Normalizes this vector and stores the result in dst.
     *
     * @param dst The destination vector.
     */
    void normalize(Derived* dst) const
    {
        assert(dst);
        const float* src = data();
        float* d = dst->data();

        if (dst != &self())
        {
            for (size_t i = 0; i < N; ++i)
                d[i] = src[i];
        }

        float n = lengthSquared();
        if (n == 1.0f)
            return;

        n = std::sqrt(n);
        if (n < MATH_TOLERANCE)
            return;

        float invN = 1.0f / n;
        for (size_t i = 0; i < N; ++i)
            d[i] *= invN;
    }

    /**
     * Scales all elements of this vector by the specified value.
     *
     * @param scalar The scalar value.
     */
    void scale(float scalar)
    {
        float* d = data();
        for (size_t i = 0; i < N; ++i)
            d[i] *= scalar;
    }

    /**
     * Adds the elements of the specified vector to this one.
     *
     * @param v The vector to add.
     */
    void add(const Derived& v)
    {
        float* d = data();
        const float* vd = v.data();
        for (size_t i = 0; i < N; ++i)
            d[i] += vd[i];
    }

    /**
     * Subtracts the specified vector from this one.
     *
     * @param v The vector to subtract.
     */
    void subtract(const Derived& v)
    {
        float* d = data();
        const float* vd = v.data();
        for (size_t i = 0; i < N; ++i)
            d[i] -= vd[i];
    }

    /**
     * Updates this vector towards the given target using a smoothing function.
     *
     * @param target Target value.
     * @param elapsedTime Elapsed time between calls.
     * @param responseTime Response time (in the same units as elapsedTime).
     */
    void smooth(const Derived& target, float elapsedTime, float responseTime)
    {
        if (elapsedTime > 0.0f)
        {
            float factor = elapsedTime / (elapsedTime + responseTime);
            float* d = data();
            const float* td = target.data();
            for (size_t i = 0; i < N; ++i)
                d[i] += (td[i] - d[i]) * factor;
        }
    }

    // ==================== Operators ====================

    /**
     * Calculates the sum of this vector with the given vector.
     */
    Derived operator+(const Derived& v) const
    {
        Derived result(self());
        result.add(v);
        return result;
    }

    /**
     * Adds the given vector to this vector.
     */
    Derived& operator+=(const Derived& v)
    {
        add(v);
        return self();
    }

    /**
     * Calculates the difference of this vector with the given vector.
     */
    Derived operator-(const Derived& v) const
    {
        Derived result(self());
        result.subtract(v);
        return result;
    }

    /**
     * Subtracts the given vector from this vector.
     */
    Derived& operator-=(const Derived& v)
    {
        subtract(v);
        return self();
    }

    /**
     * Calculates the negation of this vector.
     */
    Derived operator-() const
    {
        Derived result(self());
        result.negate();
        return result;
    }

    /**
     * Calculates the scalar product of this vector with the given value.
     */
    Derived operator*(float scalar) const
    {
        Derived result(self());
        result.scale(scalar);
        return result;
    }

    /**
     * Scales this vector by the given value.
     */
    Derived& operator*=(float scalar)
    {
        scale(scalar);
        return self();
    }

    /**
     * Returns the components of this vector divided by the given constant.
     */
    Derived operator/(float scalar) const
    {
        Derived result(self());
        float invScalar = 1.0f / scalar;
        result.scale(invScalar);
        return result;
    }

    /**
     * Divides this vector by the given value.
     */
    Derived& operator/=(float scalar)
    {
        float invScalar = 1.0f / scalar;
        scale(invScalar);
        return self();
    }

    /**
     * Determines if this vector is equal to the given vector.
     */
    bool operator==(const Derived& v) const
    {
        const float* d1 = data();
        const float* d2 = v.data();
        for (size_t i = 0; i < N; ++i)
        {
            if (d1[i] != d2[i])
                return false;
        }
        return true;
    }

    /**
     * Determines if this vector is not equal to the given vector.
     */
    bool operator!=(const Derived& v) const { return !(*this == v); }

  protected:
    VectorBase() = default;
    ~VectorBase() = default;

  private:
    Derived& self() noexcept { return *static_cast<Derived*>(this); }
    const Derived& self() const noexcept { return *static_cast<const Derived*>(this); }
};

/**
 * Calculates the scalar product of the given vector with the given value (commutative).
 */
template <typename Derived, size_t N>
inline Derived operator*(float scalar, const VectorBase<Derived, N>& v)
{
    return static_cast<const Derived&>(v) * scalar;
}

} // namespace tractor

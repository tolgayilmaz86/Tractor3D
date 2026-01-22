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

#include <memory>
#include <string>

#include "core/Factory.h"

namespace tractor
{

class Curve;

/** Shared pointer type for Curve. */
using CurvePtr = std::shared_ptr<Curve>;

/** Weak pointer type for Curve. */
using CurveWeakPtr = std::weak_ptr<Curve>;

/**
 * Defines an n-dimensional curve.
 * 
 * This class uses std::shared_ptr for memory management. Use Curve::create()
 * to instantiate new curves.
 */
class Curve : public StandaloneCreatableWithKey<Curve>
{
    friend class AnimationTarget;
    friend class Animation;
    friend class AnimationClip;
    friend class AnimationController;
    friend class MeshSkin;

  public:
    /**
     * Types of interpolation.
     *
     * Defines how the points in the curve are connected.
     *
     * Note: InterpolationType::BEZIER requires control points and InterpolationType::HERMITE requires tangents.
     */
    enum InterpolationType
    {
        /**
         * Bezier Interpolation.
         *
         * Requires that two control points are set for each segment.
         */
        BEZIER,

        /**
         * B-Spline Interpolation.
         *
         * Uses the points as control points, and the curve is guaranteed to only pass through the
         * first and last point.
         */
        BSPLINE,

        /**
         * Flat Interpolation.
         *
         * A form of Hermite interpolation that generates flat tangents for you. The tangents have a
         * value equal to 0.
         */
        FLAT,
        HERMITE,
        LINEAR,
        SMOOTH,
        STEP,
        QUADRATIC_IN,
        QUADRATIC_OUT,
        QUADRATIC_IN_OUT,
        QUADRATIC_OUT_IN,
        CUBIC_IN,
        CUBIC_OUT,
        CUBIC_IN_OUT,
        CUBIC_OUT_IN,
        QUARTIC_IN,
        QUARTIC_OUT,
        QUARTIC_IN_OUT,
        QUARTIC_OUT_IN,
        QUINTIC_IN,
        QUINTIC_OUT,
        QUINTIC_IN_OUT,
        QUINTIC_OUT_IN,
        SINE_IN,
        SINE_OUT,
        SINE_IN_OUT,
        SINE_OUT_IN,
        EXPONENTIAL_IN,
        EXPONENTIAL_OUT,
        EXPONENTIAL_IN_OUT,
        EXPONENTIAL_OUT_IN,
        CIRCULAR_IN,
        CIRCULAR_OUT,
        CIRCULAR_IN_OUT,
        CIRCULAR_OUT_IN,
        ELASTIC_IN,
        ELASTIC_OUT,
        ELASTIC_IN_OUT,
        ELASTIC_OUT_IN,
        OVERSHOOT_IN,
        OVERSHOOT_OUT,
        OVERSHOOT_IN_OUT,
        OVERSHOOT_OUT_IN,
        BOUNCE_IN,
        BOUNCE_OUT,
        BOUNCE_IN_OUT,
        BOUNCE_OUT_IN
    };

    /**
     * Constructor - requires FactoryKey, use Curve::create() instead.
     *
     * @param key Factory key (only obtainable via create())
     * @param pointCount The number of points in the curve.
     * @param componentCount The number of float component values per key value.
     */
    Curve(FactoryKey key, unsigned int pointCount, unsigned int componentCount);

    /**
     * Destructor.
     */
    ~Curve();

    // Note: create() is inherited from StandaloneCreatableWithKey<Curve>
    // Usage: auto curve = Curve::create(pointCount, componentCount);

    /**
     * Gets the number of points in the curve.
     *
     * @return The number of points in the curve.
     */
    unsigned int getPointCount() const noexcept;

    /**
     * Gets the number of float component values per points.
     *
     * @return The number of float component values per point.
     */
    unsigned int getComponentCount() const noexcept;

    /**
     * Returns the start time for the curve.
     *
     * @return The curve's start time.
     */
    float getStartTime() const noexcept;

    /**
     * Returns the end time for the curve.
     *
     * @return The curve's end time.
     */
    float getEndTime() const noexcept;

    /**
     * Sets the given point values on the curve.
     *
     * @param index The index of the point.
     * @param time The time for the key.
     * @param value The point to add.
     * @param type The curve interpolation type.
     */
    void setPoint(unsigned int index, float time, float* value, InterpolationType type);

    /**
     * Sets the given point on the curve for the specified index and the specified parameters.
     *
     * @param index The index of the point.
     * @param time The time of the point within the curve.
     * @param value The value of the point to copy the data from.
     * @param type The curve interpolation type.
     * @param inValue The tangent approaching the point.
     * @param outValue The tangent leaving the point.
     */
    void setPoint(unsigned int index,
                  float time,
                  float* value,
                  InterpolationType type,
                  float* inValue,
                  float* outValue);

    /**
     * Sets the tangents for a point on the curve specified by the index.
     *
     * @param index The index of the point.
     * @param type The interpolation type.
     * @param type The curve interpolation type.
     * @param inValue The tangent approaching the point.
     * @param outValue The tangent leaving the point.
     */
    void setTangent(unsigned int index, InterpolationType type, float* inValue, float* outValue);

    /**
     * Gets the time at a specified point.
     *
     * @param index The index of the point.
     *
     * @return The time for a key point.
     */
    float getPointTime(unsigned int index) const;

    /**
     * Gets the interpolation type at the specified point
     *
     * @param index The index of the point.
     *
     * @return The interpolation type at the specified index.
     */
    InterpolationType getPointInterpolation(unsigned int index) const;

    /**
     * Gets the values and in/out tangent value at a spedified point.
     *
     * @param index The index of the point.
     * @param value The value at the specified index. Ignored if nullptr.
     * @param inValue The tangent inValue at the specified index. Ignored if nullptr.
     * @param outValue The tangent outValue at the specified index. Ignored if nullptr.
     */
    void getPointValues(unsigned int index, float* value, float* inValue, float* outValue) const;

    /**
     * Evaluates the curve at the given position value.
     *
     * Time should generally be specified as a value between 0.0 - 1.0, inclusive.
     * A value outside this range can also be specified to perform an interpolation
     * between the two end points of the curve. This can be useful for smoothly
     * interpolating a repeat of the curve.
     *
     * @param time The position to evaluate the curve at.
     * @param dst The evaluated value of the curve at the given time.
     */
    void evaluate(float time, float* dst) const;

    /**
     * Evaluates the curve at the given position value (between 0.0 and 1.0 inclusive)
     * within the specified subregion of the curve.
     *
     * This method is useful for evaluating sub sections of the curve. A common use for
     * this is when evaluating individual animation clips that are positioned within a
     * larger animation curve. This method also allows looping to occur between the
     * end points of curve sub regions, with optional blending/interpolation between
     * the end points (using the loopBlendTime parameter).
     *
     * Time should generally be specified as a value between 0.0 - 1.0, inclusive.
     * A value outside this range can also be specified to perform an interpolation
     * between the two end points of the curve. This can be useful for smoothly
     * interpolating a repeat of the curve.
     *
     * @param time The position within the subregion of the curve to evaluate the curve at.
     *      A time of zero represents the start of the subregion, with a time of one
     *      representing the end of the subregion.
     * @param startTime Start time for the subregion (between 0.0 - 1.0).
     * @param endTime End time for the subregion (between 0.0 - 1.0).
     * @param loopBlendTime Time (in milliseconds) to blend between the end points of the curve
     *      for looping purposes when time is outside the range 0-1. A value of zero here
     *      disables curve looping.
     * @param dst The evaluated value of the curve at the given time.
     */
    void evaluate(float time, float startTime, float endTime, float loopBlendTime, float* dst) const;

    /**
     * Linear interpolation function.
     */
    static float lerp(float t, float from, float to);

  private:
    /**
     * Defines a single point within a curve.
     */
    class Point
    {
      public:
        /** The time of the point within the curve. */
        float time{0.0f};
        /** The value of the point. */
        float* value{nullptr};
        /** The value of the tangent when approaching this point (from the previous point in the curve). */
        float* inValue{nullptr};
        /** The value of the tangent when leaving this point (towards the next point in the curve). */
        float* outValue{nullptr};
        /** The type of interpolation to use between this point and the next point. */
        InterpolationType type{InterpolationType::LINEAR};

        /**
         * Constructor.
         */
        Point() = default;

        /**
         * Destructor.
         */
        ~Point();

        /**
         * Hidden copy assignment operator.
         */
        Point& operator=(const Point&) = delete;
    };

    /**
     * Copy constructor (deleted).
     */
    Curve(const Curve& copy) = delete;

    /**
     * Hidden copy assignment operator.
     */
    Curve& operator=(const Curve&) = delete;

    /**
     * Bezier interpolation function.
     */
    void interpolateBezier(float s, Point* from, Point* to, float* dst) const;

    /**
     * Bspline interpolation function.
     */
    void interpolateBSpline(float s, Point* c0, Point* c1, Point* c2, Point* c3, float* dst) const;

    /**
     * Hermite interpolation function.
     */
    void interpolateHermite(float s, Point* from, Point* to, float* dst) const;

    /**
     * Hermite interpolation function.
     */
    void interpolateHermiteFlat(float s, Point* from, Point* to, float* dst) const;

    /**
     * Hermite interpolation function.
     */
    void interpolateHermiteSmooth(float s, unsigned int index, Point* from, Point* to, float* dst) const;

    /**
     * Linear interpolation function.
     */
    void interpolateLinear(float s, Point* from, Point* to, float* dst) const;

    /**
     * Quaternion interpolation function.
     */
    void interpolateQuaternion(float s, float* from, float* to, float* dst) const;

    /**
     * Determines the current keyframe to interpolate from based on the specified time.
     */
    int determineIndex(float time, unsigned int min, unsigned int max) const;

    /**
     * Sets the offset for the beginning of a Quaternion piece of data within the curve's value span
     * at the specified index. The next four components of data starting at the given index will be
     * interpolated as a Quaternion. This function will assert an error if the given index is
     * greater than the component size subtracted by the four components required to store a
     * quaternion.
     *
     * @param index The index of the Quaternion rotation data.
     */
    void setQuaternionOffset(unsigned int index);

    /**
     * Gets the InterpolationType value for the given string ID
     *
     * @param interpolationId The string representation of the InterpolationType
     * @return the InterpolationType value; -1 if the string does not represent an InterpolationType.
     */
    static int getInterpolationType(const std::string& interpolationId);

    unsigned int _pointCount{ 0 };              // Number of points on the curve.
    unsigned int _componentCount{ 0 };          // Number of components on the curve.
    unsigned int _componentSize{ 0 };           // The component size (in bytes).
    unsigned int* _quaternionOffset{ nullptr }; // Offset for the rotation component.
    Point* _points{ nullptr };                  // The points on the curve.
};

} // namespace tractor

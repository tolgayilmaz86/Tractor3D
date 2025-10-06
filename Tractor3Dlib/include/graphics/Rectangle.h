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

namespace tractor
{

/**
 * Defines a rectangle.
 */
class Rectangle
{
  public:
    /**
     * Specifies the x-coordinate of the rectangle.
     */
    float x{ 0.0f };

    /**
     * Specifies the y-coordinate of the rectangle.
     */
    float y{ 0.0f };

    /**
     * Specifies the width of the rectangle.
     */
    float width{ 0.0f };

    /**
     * Specifies the height of the rectangle.
     */
    float height{ 0.0f };

    /**
     * Constructs a new rectangle initialized to all zeros.
     */
    Rectangle() = default;

    /**
     * Constructs a new rectangle with the x = 0, y = 0 and the specified width and height.
     *
     * @param width The width of the rectangle.
     * @param height The height of the rectangle.
     */
    Rectangle(float width, float height);

    /**
     * Constructs a new rectangle with the specified x, y, width and height.
     *
     * @param x The x-coordinate of the rectangle.
     * @param y The y-coordinate of the rectangle.
     * @param width The width of the rectangle.
     * @param height The height of the rectangle.
     */
    Rectangle(float x, float y, float width, float height);

    /**
     * Constructs a new rectangle that is a copy of the specified rectangle.
     *
     * @param copy The rectangle to copy.
     */
    Rectangle(const Rectangle& copy);

    /**
     * Destructor.
     */
    ~Rectangle() = default;

    /**
     * Returns a rectangle with all of its values set to zero.
     *
     * @return The empty rectangle with all of its values set to zero.
     */
    static const Rectangle& empty();

    /**
     * Gets a value that indicates whether the rectangle is empty.
     *
     * @return true if the rectangle is empty, false otherwise.
     */
    bool isEmpty() const noexcept { return (x == 0 && y == 0 && width == 0 && height == 0); }

    /**
     * Sets the values of this rectangle to the specified values.
     *
     * @param x The x-coordinate of the rectangle.
     * @param y The y-coordinate of the rectangle.
     * @param width The width of the rectangle.
     * @param height The height of the rectangle.
     */
    void set(float x, float y, float width, float height);

    /**
     * Sets the values of this rectangle to those in the specified rectangle.
     *
     * @param r The rectangle to copy.
     */
    void set(const Rectangle& r) { set(r.x, r.y, r.width, r.height); }

    /**
     * Sets the x-coordinate and y-coordinate values of this rectangle to the specified values.
     *
     * @param x The x-coordinate of the rectangle.
     * @param y The y-coordinate of the rectangle.
     */
    void setPosition(float x, float y);

    /**
     * Returns the x-coordinate of the left side of the rectangle.
     *
     * @return The x-coordinate of the left side of the rectangle.
     */
    float left() const noexcept { return x; }

    /**
     * Returns the y-coordinate of the top of the rectangle.
     *
     * @return The y-coordinate of the top of the rectangle.
     */
    float top() const noexcept { return y; }

    /**
     * Returns the x-coordinate of the right side of the rectangle.
     *
     * @return The x-coordinate of the right side of the rectangle.
     */
    float right() const noexcept { return x + width; }

    /**
     * Returns the y-coordinate of the bottom of the rectangle.
     *
     * @return The y-coordinate of the bottom of the rectangle.
     */
    float bottom() const noexcept { return y + height; }

    /**
     * Determines whether this rectangle contains a specified point.
     *
     * @param x The x-coordinate of the point.
     * @param y The y-coordinate of the point.
     *
     * @return true if the rectangle contains the point, false otherwise.
     */
    bool contains(float x, float y) const;

    /**
     * Determines whether this rectangle contains a specified rectangle.
     *
     * @param x The x-coordinate of the rectangle.
     * @param y The y-coordinate of the rectangle.
     * @param width The width of the rectangle.
     * @param height The height of the rectangle.
     *
     * @return true if the rectangle contains the specified rectangle, false
     * otherwise.
     */
    bool contains(float x, float y, float width, float height) const;

    /**
     * Determines whether this rectangle contains a specified rectangle.
     *
     * @param r The rectangle.
     *
     * @return true if the rectangle contains the specified rectangle, false
     * otherwise.
     */
    bool contains(const Rectangle& r) const { return contains(r.x, r.y, r.width, r.height); }

    /**
     * Determines whether a specified rectangle intersects with this rectangle.
     * Rectangles intersect if there is a common point that is contained in both rectangles.
     *
     * @param x The x-coordinate of the rectangle.
     * @param y The y-coordinate of the rectangle.
     * @param width The width of the rectangle.
     * @param height The height of the rectangle.
     *
     * @return true if the specified Rectangle intersects with this one, false otherwise.
     */
    bool intersects(float x, float y, float width, float height) const;

    /**
     * Determines whether a specified rectangle intersects with this rectangle.
     *
     * @param r The rectangle.
     *
     * @return true if the specified rectangle intersects with this one, false
     * otherwise.
     */
    bool intersects(const Rectangle& r) const;

    /**
     * Computes the intersection of two rectangles and returns the result.
     *
     * @param r1 The first rectangle.
     * @param r2 The second rectangle.
     * @param dst Populated with the resulting intersection, or Rectangle.empty if they do not intersect.
     *
     * @return True if the two rectangles intersect, false otherwise.
     */
    static bool intersect(const Rectangle& r1, const Rectangle& r2, Rectangle* dst);

    /**
     * Returns a new rectangle that exactly contains two other rectangles.
     *
     * @param r1 The first rectangle to contain.
     * @param r2 The second rectangle to contain.
     * @param dst A rectangle to store the union of the two rectangle parameters.
     */
    static void combine(const Rectangle& r1, const Rectangle& r2, Rectangle* dst);

    /**
     * Pushes the edges of the Rectangle out by the horizontal and vertical values specified.
     *
     * Each corner of the Rectangle is pushed away from the center of the rectangle
     * by the specified amounts. This results in the width and height of the Rectangle
     * increasing by twice the values provided.
     *
     * @param horizontalAmount The value to push the sides out by.
     * @param verticalAmount The value to push the top and bottom out by.
     */
    void inflate(float horizontalAmount, float verticalAmount);

    /**
     * operator =
     */
    Rectangle& operator=(const Rectangle& r);

    /**
     * operator ==
     */
    bool operator==(const Rectangle& r) const;

    /**
     * operator !=
     */
    bool operator!=(const Rectangle& r) const;
};

} // namespace tractor

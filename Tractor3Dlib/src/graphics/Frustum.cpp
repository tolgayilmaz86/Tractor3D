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

#include "graphics/Frustum.h"

#include "graphics/BoundingBox.h"
#include "graphics/BoundingSphere.h"

namespace tractor
{

//----------------------------------------------------------------------------
Frustum::Frustum(const Matrix& matrix) { set(matrix); }

//----------------------------------------------------------------------------
Frustum::Frustum(const Frustum& frustum) { set(frustum); }

//----------------------------------------------------------------------------
void Frustum::getMatrix(Matrix* dst) const
{
    assert(dst);
    dst->set(_matrix);
}

//----------------------------------------------------------------------------
void Frustum::getCorners(Vector3* corners) const
{
    getNearCorners(corners);
    getFarCorners(corners + 4);
}

//----------------------------------------------------------------------------
void Frustum::getNearCorners(Vector3* corners) const
{
    assert(corners);

    Plane::intersection(_near, _left, _top, &corners[0]);
    Plane::intersection(_near, _left, _bottom, &corners[1]);
    Plane::intersection(_near, _right, _bottom, &corners[2]);
    Plane::intersection(_near, _right, _top, &corners[3]);
}

//----------------------------------------------------------------------------
void Frustum::getFarCorners(Vector3* corners) const
{
    assert(corners);

    Plane::intersection(_far, _right, _top, &corners[0]);
    Plane::intersection(_far, _right, _bottom, &corners[1]);
    Plane::intersection(_far, _left, _bottom, &corners[2]);
    Plane::intersection(_far, _left, _top, &corners[3]);
}

//----------------------------------------------------------------------------
bool Frustum::intersects(const Vector3& point) const noexcept
{
    if (_near.distance(point) <= 0) return false;
    if (_far.distance(point) <= 0) return false;
    if (_left.distance(point) <= 0) return false;
    if (_right.distance(point) <= 0) return false;
    if (_top.distance(point) <= 0) return false;
    if (_bottom.distance(point) <= 0) return false;

    return true;
}

//----------------------------------------------------------------------------
bool Frustum::intersects(float x, float y, float z) const noexcept { return intersects(Vector3(x, y, z)); }

//----------------------------------------------------------------------------
bool Frustum::intersects(const BoundingSphere& sphere) const noexcept { return sphere.intersects(*this); }

//----------------------------------------------------------------------------
bool Frustum::intersects(const BoundingBox& box) const noexcept { return box.intersects(*this); }

//----------------------------------------------------------------------------
float Frustum::intersects(const Plane& plane) const noexcept { return plane.intersects(*this); }

//----------------------------------------------------------------------------
float Frustum::intersects(const Ray& ray) const noexcept { return ray.intersects(*this); }

//----------------------------------------------------------------------------
void Frustum::set(const Frustum& frustum)
{
    _near = frustum._near;
    _far = frustum._far;
    _bottom = frustum._bottom;
    _top = frustum._top;
    _left = frustum._left;
    _right = frustum._right;
    _matrix.set(frustum._matrix);
}

//----------------------------------------------------------------------------
void Frustum::updatePlanes()
{
    _near.set(Vector3(_matrix.m[3] + _matrix.m[2],
                      _matrix.m[7] + _matrix.m[6],
                      _matrix.m[11] + _matrix.m[10]),
              _matrix.m[15] + _matrix.m[14]);
    _far.set(Vector3(_matrix.m[3] - _matrix.m[2],
                     _matrix.m[7] - _matrix.m[6],
                     _matrix.m[11] - _matrix.m[10]),
             _matrix.m[15] - _matrix.m[14]);
    _bottom.set(Vector3(_matrix.m[3] + _matrix.m[1],
                        _matrix.m[7] + _matrix.m[5],
                        _matrix.m[11] + _matrix.m[9]),
                _matrix.m[15] + _matrix.m[13]);
    _top.set(Vector3(_matrix.m[3] - _matrix.m[1],
                     _matrix.m[7] - _matrix.m[5],
                     _matrix.m[11] - _matrix.m[9]),
             _matrix.m[15] - _matrix.m[13]);
    _left.set(Vector3(_matrix.m[3] + _matrix.m[0],
                      _matrix.m[7] + _matrix.m[4],
                      _matrix.m[11] + _matrix.m[8]),
              _matrix.m[15] + _matrix.m[12]);
    _right.set(Vector3(_matrix.m[3] - _matrix.m[0],
                       _matrix.m[7] - _matrix.m[4],
                       _matrix.m[11] - _matrix.m[8]),
               _matrix.m[15] - _matrix.m[12]);
}

//----------------------------------------------------------------------------
void Frustum::set(const Matrix& matrix)
{
    _matrix.set(matrix);

    // Update the planes.
    updatePlanes();
}

} // namespace tractor

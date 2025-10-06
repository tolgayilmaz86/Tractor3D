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

#include "graphics/Light.h"

#include "scene/Node.h"

namespace tractor
{

//----------------------------------------------------------------------------
Light::Light(Light::Type type, const Vector3& color) : _type(type), _node(nullptr)
{
    _directional = new Directional(color);
}

//----------------------------------------------------------------------------
Light::Light(Light::Type type, const Vector3& color, float range) : _type(type), _node(nullptr)
{
    _point = new Point(color, range);
}

//----------------------------------------------------------------------------
Light::Light(Light::Type type, const Vector3& color, float range, float innerAngle, float outerAngle)
    : _type(type), _node(nullptr)
{
    _spot = new Spot(color, range, innerAngle, outerAngle);
}

//----------------------------------------------------------------------------
Light::~Light()
{
    switch (_type)
    {
        case DIRECTIONAL:
            SAFE_DELETE(_directional);
            break;
        case POINT:
            SAFE_DELETE(_point);
            break;
        case SPOT:
            SAFE_DELETE(_spot);
            break;
        default:
            GP_ERROR("Invalid light type (%d).", _type);
            break;
    }
}

//----------------------------------------------------------------------------
Light* Light::createDirectional(const Vector3& color) { return new Light(DIRECTIONAL, color); }

//----------------------------------------------------------------------------
Light* Light::createDirectional(float red, float green, float blue)
{
    return new Light(DIRECTIONAL, Vector3(red, green, blue));
}

//----------------------------------------------------------------------------
Light* Light::createPoint(const Vector3& color, float range)
{
    return new Light(POINT, color, range);
}

//----------------------------------------------------------------------------
Light* Light::createPoint(float red, float green, float blue, float range)
{
    return new Light(POINT, Vector3(red, green, blue), range);
}

//----------------------------------------------------------------------------
Light* Light::createSpot(const Vector3& color, float range, float innerAngle, float outerAngle)
{
    return new Light(SPOT, color, range, innerAngle, outerAngle);
}

//----------------------------------------------------------------------------
Light* Light::createSpot(float red, float green, float blue, float range, float innerAngle, float outerAngle)
{
    return new Light(SPOT, Vector3(red, green, blue), range, innerAngle, outerAngle);
}

//----------------------------------------------------------------------------
Light* Light::create(Properties* properties)
{
    assert(properties);

    // Read light type
    std::string typeStr;
    if (properties->exists("type")) typeStr = properties->getString("type");
    Light::Type type;
    if (typeStr == "DIRECTIONAL")
    {
        type = Light::DIRECTIONAL;
    }
    else if (typeStr == "POINT")
    {
        type = Light::POINT;
    }
    else if (typeStr == "SPOT")
    {
        type = Light::SPOT;
    }
    else
    {
        GP_ERROR("Invalid 'type' parameter for light definition.");
        return nullptr;
    }

    // Read common parameters
    Vector3 color;
    if (!properties->getVector3("color", &color))
    {
        GP_ERROR("Missing valid 'color' parameter for light definition.");
        return nullptr;
    }

    // Read light-specific parameters
    Light* light = nullptr;
    switch (type)
    {
        case DIRECTIONAL:
            light = createDirectional(color);
            break;
        case POINT:
        {
            float range = properties->getFloat("range");
            if (range == 0.0f)
            {
                GP_ERROR("Missing valid 'range' parameter for point light definition.");
                return nullptr;
            }
            light = createPoint(color, range);
        }
        break;
        case SPOT:
            float range = properties->getFloat("range");
            if (range == 0.0f)
            {
                GP_ERROR("Missing valid 'range' parameter for spot light definition.");
                return nullptr;
            }
            float innerAngle = properties->getFloat("innerAngle");
            if (innerAngle == 0.0f)
            {
                GP_ERROR("Missing valid 'innerAngle' parameter for spot light definition.");
                return nullptr;
            }
            float outerAngle = properties->getFloat("outerAngle");
            if (outerAngle == 0.0f)
            {
                GP_ERROR("Missing valid 'outerAngle' parameter for spot light definition.");
                return nullptr;
            }
            light = createSpot(color, range, innerAngle, outerAngle);
            break;
    }

    return light;
}

//----------------------------------------------------------------------------
const Vector3& Light::getColor() const
{
    switch (_type)
    {
        case DIRECTIONAL:
            assert(_directional);
            return _directional->color;
        case POINT:
            assert(_point);
            return _point->color;
        case SPOT:
            assert(_spot);
            return _spot->color;
        default:
            GP_ERROR("Unsupported light type (%d).", _type);
            return Vector3::zero();
    }
}

//----------------------------------------------------------------------------
void Light::setColor(const Vector3& color)
{
    switch (_type)
    {
        case DIRECTIONAL:
            assert(_directional);
            _directional->color = color;
            break;
        case POINT:
            assert(_point);
            _point->color = color;
            break;
        case SPOT:
            assert(_spot);
            _spot->color = color;
            break;
        default:
            GP_ERROR("Unsupported light type (%d).", _type);
            break;
    }
}

//----------------------------------------------------------------------------
void Light::setColor(float red, float green, float blue) { setColor(Vector3(red, green, blue)); }

//----------------------------------------------------------------------------
float Light::getRange() const
{
    assert(_type != DIRECTIONAL);

    switch (_type)
    {
        case POINT:
            assert(_point);
            return _point->range;
        case SPOT:
            assert(_spot);
            return _spot->range;
        default:
            GP_ERROR("Unsupported light type (%d).", _type);
            return 0.0f;
    }
}

//----------------------------------------------------------------------------
void Light::setRange(float range)
{
    assert(_type != DIRECTIONAL);

    switch (_type)
    {
        case POINT:
            assert(_point);
            _point->range = range;
            _point->rangeInverse = 1.0f / range;
            break;
        case SPOT:
            assert(_spot);
            _spot->range = range;
            _spot->rangeInverse = 1.0f / range;
            break;
        default:
            GP_ERROR("Unsupported light type (%d).", _type);
            break;
    }

    if (_node) _node->setBoundsDirty();
}

//----------------------------------------------------------------------------
float Light::getRangeInverse() const
{
    assert(_type != DIRECTIONAL);

    switch (_type)
    {
        case POINT:
            assert(_point);
            return _point->rangeInverse;
        case SPOT:
            assert(_spot);
            return _spot->rangeInverse;
        default:
            GP_ERROR("Unsupported light type (%d).", _type);
            return 0.0f;
    }
}

//----------------------------------------------------------------------------
float Light::getInnerAngle() const
{
    assert(_type == SPOT);

    return _spot->innerAngle;
}

//----------------------------------------------------------------------------
void Light::setInnerAngle(float innerAngle)
{
    assert(_type == SPOT);

    _spot->innerAngle = innerAngle;
    _spot->innerAngleCos = cos(innerAngle);
}

//----------------------------------------------------------------------------
float Light::getOuterAngle() const
{
    assert(_type == SPOT);

    return _spot->outerAngle;
}

//----------------------------------------------------------------------------
void Light::setOuterAngle(float outerAngle)
{
    assert(_type == SPOT);

    _spot->outerAngle = outerAngle;
    _spot->outerAngleCos = cos(outerAngle);

    if (_node) _node->setBoundsDirty();
}

//----------------------------------------------------------------------------
float Light::getInnerAngleCos() const
{
    assert(_type == SPOT);

    return _spot->innerAngleCos;
}

//----------------------------------------------------------------------------
float Light::getOuterAngleCos() const
{
    assert(_type == SPOT);

    return _spot->outerAngleCos;
}

//----------------------------------------------------------------------------
Light* Light::clone(NodeCloneContext& context)
{
    Light* lightClone = nullptr;
    switch (_type)
    {
        case DIRECTIONAL:
            lightClone = createDirectional(getColor());
            break;
        case POINT:
            lightClone = createPoint(getColor(), getRange());
            break;
        case SPOT:
            lightClone = createSpot(getColor(), getRange(), getInnerAngle(), getOuterAngle());
            break;
        default:
            GP_ERROR("Unsupported light type (%d).", _type);
            return nullptr;
    }
    assert(lightClone);

    if (Node* node = context.findClonedNode(getNode()))
    {
        lightClone->setNode(node);
    }
    return lightClone;
}

//----------------------------------------------------------------------------
Light::Directional::Directional(const Vector3& color) : color(color) {}

//----------------------------------------------------------------------------
Light::Point::Point(const Vector3& color, float range) : color(color), range(range)
{
    rangeInverse = 1.0f / range;
}

//----------------------------------------------------------------------------
Light::Spot::Spot(const Vector3& color, float range, float innerAngle, float outerAngle)
    : color(color), range(range), innerAngle(innerAngle), outerAngle(outerAngle)
{
    rangeInverse = 1.0f / range;
    innerAngleCos = cos(innerAngle);
    outerAngleCos = cos(outerAngle);
}

} // namespace tractor

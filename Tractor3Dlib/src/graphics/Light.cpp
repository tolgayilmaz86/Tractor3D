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
Light::Light(Light::Type type, const Vector3& color) 
    : _type(type), _lightData(Directional(color)), _node(nullptr)
{
}

//----------------------------------------------------------------------------
Light::Light(Light::Type type, const Vector3& color, float range) 
    : _type(type), _lightData(Point(color, range)), _node(nullptr)
{
}

//----------------------------------------------------------------------------
Light::Light(Light::Type type, const Vector3& color, float range, float innerAngle, float outerAngle)
    : _type(type), _lightData(Spot(color, range, innerAngle, outerAngle)), _node(nullptr)
{
}

//----------------------------------------------------------------------------
LightPtr Light::createDirectional(const Vector3& color) 
{ 
    return LightPtr(new Light(DIRECTIONAL, color)); 
}

//----------------------------------------------------------------------------
LightPtr Light::createDirectional(float red, float green, float blue)
{
    return LightPtr(new Light(DIRECTIONAL, Vector3(red, green, blue)));
}

//----------------------------------------------------------------------------
LightPtr Light::createPoint(const Vector3& color, float range)
{
    return LightPtr(new Light(POINT, color, range));
}

//----------------------------------------------------------------------------
LightPtr Light::createPoint(float red, float green, float blue, float range)
{
    return LightPtr(new Light(POINT, Vector3(red, green, blue), range));
}

//----------------------------------------------------------------------------
LightPtr Light::createSpot(const Vector3& color, float range, float innerAngle, float outerAngle)
{
    return LightPtr(new Light(SPOT, color, range, innerAngle, outerAngle));
}

//----------------------------------------------------------------------------
LightPtr Light::createSpot(float red, float green, float blue, float range, float innerAngle, float outerAngle)
{
    return LightPtr(new Light(SPOT, Vector3(red, green, blue), range, innerAngle, outerAngle));
}

//----------------------------------------------------------------------------
LightPtr Light::create(Properties* properties)
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
    LightPtr light = nullptr;
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
            return std::get<Directional>(_lightData).color;
        case POINT:
            return std::get<Point>(_lightData).color;
        case SPOT:
            return std::get<Spot>(_lightData).color;
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
            std::get<Directional>(_lightData).color = color;
            break;
        case POINT:
            std::get<Point>(_lightData).color = color;
            break;
        case SPOT:
            std::get<Spot>(_lightData).color = color;
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
            return std::get<Point>(_lightData).range;
        case SPOT:
            return std::get<Spot>(_lightData).range;
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
        {
            auto& point = std::get<Point>(_lightData);
            point.range = range;
            point.rangeInverse = 1.0f / range;
            break;
        }
        case SPOT:
        {
            auto& spot = std::get<Spot>(_lightData);
            spot.range = range;
            spot.rangeInverse = 1.0f / range;
            break;
        }
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
            return std::get<Point>(_lightData).rangeInverse;
        case SPOT:
            return std::get<Spot>(_lightData).rangeInverse;
        default:
            GP_ERROR("Unsupported light type (%d).", _type);
            return 0.0f;
    }
}

//----------------------------------------------------------------------------
float Light::getInnerAngle() const
{
    assert(_type == SPOT);

    return std::get<Spot>(_lightData).innerAngle;
}

//----------------------------------------------------------------------------
void Light::setInnerAngle(float innerAngle)
{
    assert(_type == SPOT);

    auto& spot = std::get<Spot>(_lightData);
    spot.innerAngle = innerAngle;
    spot.innerAngleCos = cos(innerAngle);
}

//----------------------------------------------------------------------------
float Light::getOuterAngle() const
{
    assert(_type == SPOT);

    return std::get<Spot>(_lightData).outerAngle;
}

//----------------------------------------------------------------------------
void Light::setOuterAngle(float outerAngle)
{
    assert(_type == SPOT);

    auto& spot = std::get<Spot>(_lightData);
    spot.outerAngle = outerAngle;
    spot.outerAngleCos = cos(outerAngle);

    if (_node) _node->setBoundsDirty();
}

//----------------------------------------------------------------------------
float Light::getInnerAngleCos() const
{
    assert(_type == SPOT);

    return std::get<Spot>(_lightData).innerAngleCos;
}

//----------------------------------------------------------------------------
float Light::getOuterAngleCos() const
{
    assert(_type == SPOT);

    return std::get<Spot>(_lightData).outerAngleCos;
}

//----------------------------------------------------------------------------
LightPtr Light::clone(NodeCloneContext& context)
{
    LightPtr lightClone = nullptr;
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

    if (NodePtr node = context.findClonedNode(getNode()))
    {
        lightClone->setNode(node.get());
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

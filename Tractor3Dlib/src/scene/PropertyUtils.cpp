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

#include "scene/PropertyUtils.h"

namespace tractor
{

//----------------------------------------------------------------------------
bool PropertyUtils::getColor(Properties* properties, const std::string& name, Vector4* out)
{
    if (!properties || !out)
        return false;

    if (!properties->exists(name))
        return false;

    switch (properties->getType(name))
    {
        case Properties::VECTOR3:
            out->w = 1.0f;
            return properties->getVector3(name, reinterpret_cast<Vector3*>(out));
        case Properties::VECTOR4:
            return properties->getVector4(name, out);
        case Properties::STRING:
        default:
            return properties->getColor(name, out);
    }
}

//----------------------------------------------------------------------------
bool PropertyUtils::getColor(Properties* properties, const std::string& name, Vector3* out)
{
    if (!properties || !out)
        return false;

    if (!properties->exists(name))
        return false;

    switch (properties->getType(name))
    {
        case Properties::VECTOR3:
            return properties->getVector3(name, out);
        case Properties::VECTOR4:
        {
            Vector4 v4;
            if (properties->getVector4(name, &v4))
            {
                out->set(v4.x, v4.y, v4.z);
                return true;
            }
            return false;
        }
        case Properties::STRING:
        default:
            return properties->getColor(name, out);
    }
}

//----------------------------------------------------------------------------
bool PropertyUtils::getPath(Properties* properties, const std::string& name, std::string& path)
{
    if (!properties)
        return false;

    return properties->getPath(name, path);
}

//----------------------------------------------------------------------------
float PropertyUtils::getFloat(Properties* properties, const std::string& name, float defaultValue)
{
    if (!properties || !properties->exists(name))
        return defaultValue;

    return properties->getFloat(name);
}

//----------------------------------------------------------------------------
int PropertyUtils::getInt(Properties* properties, const std::string& name, int defaultValue)
{
    if (!properties || !properties->exists(name))
        return defaultValue;

    return properties->getInt(name);
}

//----------------------------------------------------------------------------
bool PropertyUtils::getBool(Properties* properties, const std::string& name, bool defaultValue)
{
    if (!properties || !properties->exists(name))
        return defaultValue;

    return properties->getBool(name);
}

//----------------------------------------------------------------------------
bool PropertyUtils::validateNamespace(Properties* properties, 
                                      const std::string& expectedNamespace,
                                      const std::string& errorContext)
{
    if (!properties)
    {
        GP_ERROR("Properties object must be non-null for %s.", errorContext.c_str());
        return false;
    }

    if (properties->getNamespace() != expectedNamespace)
    {
        GP_ERROR("Properties object must have namespace equal to '%s' for %s.",
                 expectedNamespace.c_str(), errorContext.c_str());
        return false;
    }

    return true;
}

} // namespace tractor

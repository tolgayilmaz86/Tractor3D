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

#include "physics/PhysicsCollisionShape.h"

#include "framework/FileSystem.h"
#include "graphics/HeightField.h"
#include "graphics/Terrain.h"
#include "scene/Node.h"
#include "scene/Properties.h"
#include "ui/Image.h"

namespace tractor
{

//----------------------------------------------------------------------------
PhysicsCollisionShape::PhysicsCollisionShape(PrivateTag,
                                             Type type,
                                             btCollisionShape* shape,
                                             btStridingMeshInterface* meshInterface)
    : _type(type), _shape(shape), _meshInterface(meshInterface)
{
    memset(&_shapeData, 0, sizeof(_shapeData));
}

//----------------------------------------------------------------------------
PhysicsCollisionShape::~PhysicsCollisionShape()
{
    if (_shape)
    {
        switch (_type)
        {
            case SHAPE_MESH:
                if (_shapeData.meshData)
                {
                    delete[] _shapeData.meshData->vertexData;
                    for (size_t i = 0; i < _shapeData.meshData->indexData.size(); i++)
                    {
                        delete[] _shapeData.meshData->indexData[i];
                    }
                    delete _shapeData.meshData;
                }
                delete _meshInterface;
                break;

            case SHAPE_HEIGHTFIELD:
                if (_shapeData.heightfieldData)
                {
                    _shapeData.heightfieldData->heightfield.reset();  // shared_ptr cleanup
                    delete _shapeData.heightfieldData;
                }
                break;
        }

        delete _shape;
    }
}

//----------------------------------------------------------------------------
PhysicsCollisionShape::Definition::Definition()
{
    memset(&data, 0, sizeof(data));
}

//----------------------------------------------------------------------------
PhysicsCollisionShape::Definition::Definition(const Definition& definition)
    : type(definition.type),
      data(definition.data),
      heightfieldData(definition.heightfieldData),  // shared_ptr copy
      isExplicit(definition.isExplicit),
      centerAbsolute(definition.centerAbsolute)
{
}

//----------------------------------------------------------------------------
PhysicsCollisionShape::Definition& PhysicsCollisionShape::Definition::operator=(
    const Definition& definition)
{
    if (this != &definition)
    {
        type = definition.type;
        data = definition.data;
        heightfieldData = definition.heightfieldData;
        isExplicit = definition.isExplicit;
        centerAbsolute = definition.centerAbsolute;
    }

    return *this;
}

//----------------------------------------------------------------------------
PhysicsCollisionShape::Definition PhysicsCollisionShape::Definition::create(Node* node,
                                                                            Properties* properties)
{
    assert(node);

    if (!properties || properties->getNamespace() != "collisionObject")
    {
        GP_ERROR("Failed to load physics collision shape from properties object: must be non-null "
                 "object and have namespace equal to 'collisionObject'.");
        return Definition();
    }

    PhysicsCollisionShape::Type type = PhysicsCollisionShape::SHAPE_BOX;
    Vector3 extents, center;
    bool extentsSpecified = false;
    bool centerSpecified = false;
    float radius = -1.0f;
    float width = -1.0f;
    float height = -1.0f;
    bool centerIsAbsolute = false;
    std::string imagePath;
    float maxHeight = 0;
    float minHeight = 0;
    bool shapeSpecified = false;

    properties->rewind();
    std::string name;

    while (auto property = properties->getNextProperty())
    {
        const auto& name = property->name;

        if (name == "shape")
        {
            std::string shapeStr = properties->getString();
            if (shapeStr == "BOX")
                type = SHAPE_BOX;
            else if (shapeStr == "SPHERE")
                type = SHAPE_SPHERE;
            else if (shapeStr == "MESH")
                type = SHAPE_MESH;
            else if (shapeStr == "HEIGHTFIELD")
                type = SHAPE_HEIGHTFIELD;
            else if (shapeStr == "CAPSULE")
                type = SHAPE_CAPSULE;
            else
            {
                GP_ERROR("Could not create physics collision shape; unsupported value for "
                         "collision shape type: '%s'.",
                         shapeStr.c_str());
                return Definition();
            }

            shapeSpecified = true;
        }
        else if (name == "image")
        {
            imagePath = properties->getString();
        }
        else if (name == "maxHeight")
        {
            maxHeight = properties->getFloat();
        }
        else if (name == "minHeight")
        {
            minHeight = properties->getFloat();
        }
        else if (name == "radius")
        {
            radius = properties->getFloat();
        }
        else if (name == "width")
        {
            width = properties->getFloat();
        }
        else if (name == "height")
        {
            height = properties->getFloat();
        }
        else if (name == "extents")
        {
            properties->getVector3("extents", &extents);
            extentsSpecified = true;
        }
        else if (name == "center")
        {
            properties->getVector3("center", &center);
            centerSpecified = true;
        }
        else if (name == "centerAbsolute")
        {
            centerIsAbsolute = properties->getBool();
        }
    }

    if (!shapeSpecified)
    {
        GP_ERROR("Missing 'shape' specifier for collision shape definition.");
        return Definition();
    }

    Definition shape;
    switch (type)
    {
        case SHAPE_BOX:
            if (extentsSpecified)
            {
                if (centerSpecified)
                    shape = box(extents, center, centerIsAbsolute);
                else
                    shape = box(extents);
            }
            else
            {
                shape = box();
            }
            break;

        case SHAPE_SPHERE:
            if (radius != -1.0f)
            {
                if (centerSpecified)
                    shape = sphere(radius, center, centerIsAbsolute);
                else
                    shape = sphere(radius);
            }
            else
            {
                shape = sphere();
            }
            break;

        case SHAPE_CAPSULE:
            if (radius != -1.0f && height != -1.0f)
            {
                if (centerSpecified)
                    shape = capsule(radius, height, center, centerIsAbsolute);
                else
                    shape = capsule(radius, height);
            }
            else
            {
                shape = capsule();
            }
            break;

        case SHAPE_MESH:
        {
            Mesh* nodeMesh =
                node->getDrawable() ? dynamic_cast<Model*>(node->getDrawable())->getMesh() : nullptr;
            if (nodeMesh == nullptr)
            {
                GP_ERROR("Cannot create mesh collision object for node without model/mesh.");
            }
            else
            {
                switch (nodeMesh->getPrimitiveType())
                {
                    case Mesh::TRIANGLES:
                        shape = mesh(nodeMesh);
                        break;
                    case Mesh::LINES:
                    case Mesh::LINE_STRIP:
                    case Mesh::POINTS:
                    case Mesh::TRIANGLE_STRIP:
                        GP_ERROR("Mesh collision objects are currently only supported on meshes "
                                 "with primitive type equal to TRIANGLES.");
                        break;
                }
            }
        }
        break;

        case SHAPE_HEIGHTFIELD:
        {
            if (imagePath.empty())
            {
                if (dynamic_cast<Terrain*>(node->getDrawable()) == nullptr)
                {
                    GP_ERROR("Heightfield collision objects can only be specified on nodes that "
                             "have a valid terrain, or that specify an image path.");
                }
                else
                {
                    shape = PhysicsCollisionShape::heightfield();
                }
            }
            else
            {
                std::string ext = FileSystem::getExtension(imagePath);
                HeightFieldPtr heightfield = nullptr;
                if (ext == ".PNG")
                    heightfield = HeightField::createFromImage(imagePath, minHeight, maxHeight);
                else if (ext == ".RAW" || ext == ".R16")
                    heightfield = HeightField::createFromRAW(imagePath,
                                                             (unsigned int)width,
                                                             (unsigned int)height,
                                                             minHeight,
                                                             maxHeight);

                if (heightfield)
                {
                    shape = PhysicsCollisionShape::heightfield(heightfield.get());
                    // No need to release - shared_ptr handles it
                }
            }
        }
        break;

        default:
            GP_ERROR("Unsupported physics collision shape type (%d).", type);
            break;
    }

    return shape;
}

//----------------------------------------------------------------------------
PhysicsCollisionShape::Definition PhysicsCollisionShape::box()
{
    Definition d;
    d.type = SHAPE_BOX;
    d.isExplicit = false;
    d.centerAbsolute = false;
    return d;
}

//----------------------------------------------------------------------------
PhysicsCollisionShape::Definition PhysicsCollisionShape::box(const Vector3& extents,
                                                             const Vector3& center,
                                                             bool absolute)
{
    Definition d;
    d.type = SHAPE_BOX;
    memcpy(d.data.box.extents, &extents.x, sizeof(float) * 3);
    memcpy(d.data.box.center, &center.x, sizeof(float) * 3);
    d.isExplicit = true;
    d.centerAbsolute = absolute;
    return d;
}

//----------------------------------------------------------------------------
PhysicsCollisionShape::Definition PhysicsCollisionShape::sphere()
{
    Definition d;
    d.type = SHAPE_SPHERE;
    d.isExplicit = false;
    d.centerAbsolute = false;
    return d;
}

//----------------------------------------------------------------------------
PhysicsCollisionShape::Definition PhysicsCollisionShape::sphere(float radius,
                                                                const Vector3& center,
                                                                bool absolute)
{
    Definition d;
    d.type = SHAPE_SPHERE;
    d.data.sphere.radius = radius;
    memcpy(d.data.sphere.center, &center.x, sizeof(float) * 3);
    d.isExplicit = true;
    d.centerAbsolute = absolute;
    return d;
}

//----------------------------------------------------------------------------
PhysicsCollisionShape::Definition PhysicsCollisionShape::capsule()
{
    Definition d;
    d.type = SHAPE_CAPSULE;
    d.isExplicit = false;
    d.centerAbsolute = false;
    return d;
}

//----------------------------------------------------------------------------
PhysicsCollisionShape::Definition PhysicsCollisionShape::capsule(float radius,
                                                                 float height,
                                                                 const Vector3& center,
                                                                 bool absolute)
{
    Definition d;
    d.type = SHAPE_CAPSULE;
    d.data.capsule.radius = radius;
    d.data.capsule.height = height;
    memcpy(d.data.capsule.center, &center.x, sizeof(float) * 3);
    d.isExplicit = true;
    d.centerAbsolute = absolute;
    return d;
}

//----------------------------------------------------------------------------
PhysicsCollisionShape::Definition PhysicsCollisionShape::heightfield()
{
    Definition d;
    d.type = SHAPE_HEIGHTFIELD;
    d.isExplicit = false;
    d.centerAbsolute = false;
    return d;
}

//----------------------------------------------------------------------------
PhysicsCollisionShape::Definition PhysicsCollisionShape::heightfield(HeightField* heightfield)
{
    assert(heightfield);

    Definition d;
    d.type = SHAPE_HEIGHTFIELD;
    d.heightfieldData = heightfield->shared_from_this();  // Get shared_ptr
    d.isExplicit = true;
    d.centerAbsolute = false;
    return d;
}

//----------------------------------------------------------------------------
PhysicsCollisionShape::Definition PhysicsCollisionShape::mesh(Mesh* mesh)
{
    assert(mesh);

    Definition d;
    d.type = SHAPE_MESH;
    d.data.mesh = mesh;
    d.isExplicit = true;
    d.centerAbsolute = false;
    return d;
}

} // namespace tractor

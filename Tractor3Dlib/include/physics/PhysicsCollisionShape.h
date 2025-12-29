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

#include "graphics/HeightField.h"
#include "graphics/Mesh.h"
#include "math/Vector3.h"

namespace tractor
{
class Node;
class Properties;
class PhysicsCollisionShape;

/** Shared pointer type for PhysicsCollisionShape. */
using PhysicsCollisionShapePtr = std::shared_ptr<PhysicsCollisionShape>;

/** Weak pointer type for PhysicsCollisionShape. */
using PhysicsCollisionShapeWeakPtr = std::weak_ptr<PhysicsCollisionShape>;

/**
 * Defines the physics collision shape class that all supported shapes derive from.
 * 
 * @note PhysicsCollisionShape uses std::shared_ptr for memory management.
 */
class PhysicsCollisionShape : public std::enable_shared_from_this<PhysicsCollisionShape>
{
    friend class PhysicsController;
    friend class PhysicsRigidBody;

  public:
    enum Type
    {
        SHAPE_NONE,
        SHAPE_BOX,
        SHAPE_SPHERE,
        SHAPE_CAPSULE,
        SHAPE_MESH,
        SHAPE_HEIGHTFIELD
    };

    struct Definition
    {
        friend class PhysicsCollisionShape;
        friend class PhysicsController;
        friend class PhysicsRigidBody;
        friend class PhysicsCharacter;
        friend class PhysicsGhostObject;

      public:
        Definition();
        Definition(const Definition& definition);
        Definition& operator=(const Definition& definition);
        ~Definition() = default;

        bool isEmpty() const noexcept { return type == SHAPE_NONE; }

      private:
        static Definition create(Node* node, Properties* properties);

        PhysicsCollisionShape::Type type{ SHAPE_NONE };

        struct BoxData
        {
            float center[3], extents[3];
        };
        struct SphereData
        {
            float center[3];
            float radius;
        };
        struct CapsuleData
        {
            float center[3];
            float radius, height;
        };

        union
        {
            /** @script{ignore} */
            BoxData box;
            /** @script{ignore} */
            SphereData sphere;
            /** @script{ignore} */
            CapsuleData capsule;
            /** @script{ignore} */
            Mesh* mesh;
        } data;

        // HeightField stored separately as shared_ptr (can't be in union)
        HeightFieldPtr heightfieldData;

        bool isExplicit{ false };
        bool centerAbsolute{ false };
    };

    /**
     * Destructor.
     */
    ~PhysicsCollisionShape();

    PhysicsCollisionShape::Type getType() const noexcept { return _type; }
    btCollisionShape* getShape() const noexcept { return _shape; }

    static PhysicsCollisionShape::Definition box();
    static PhysicsCollisionShape::Definition box(const Vector3& extents,
                                                 const Vector3& center = Vector3::zero(),
                                                 bool absolute = false);
    static PhysicsCollisionShape::Definition sphere();
    static PhysicsCollisionShape::Definition sphere(float radius,
                                                    const Vector3& center = Vector3::zero(),
                                                    bool absolute = false);
    static PhysicsCollisionShape::Definition capsule();
    static PhysicsCollisionShape::Definition capsule(float radius,
                                                     float height,
                                                     const Vector3& center = Vector3::zero(),
                                                     bool absolute = false);
    static PhysicsCollisionShape::Definition heightfield();
    static PhysicsCollisionShape::Definition heightfield(HeightField* heightfield);
    static PhysicsCollisionShape::Definition mesh(Mesh* mesh);

  private:
    struct MeshData
    {
        float* vertexData;
        std::vector<unsigned char*> indexData;
    };

    struct HeightfieldData
    {
        HeightFieldPtr heightfield;
        bool inverseIsDirty;
        Matrix inverse;
        float minHeight;
        float maxHeight;
    };

    // Private struct to allow make_shared while keeping constructor effectively private
    struct PrivateTag {};

  public:
    /**
     * Constructor (use PhysicsController to create shapes).
     * @internal This constructor is public to enable std::make_shared but should not be called directly.
     */
    explicit PhysicsCollisionShape(PrivateTag,
                                   Type type,
                                   btCollisionShape* shape,
                                   btStridingMeshInterface* meshInterface = nullptr);

  private:
    PhysicsCollisionShape(const PhysicsCollisionShape& copy) = delete;
    PhysicsCollisionShape& operator=(const PhysicsCollisionShape&) = delete;

    Type _type;
    btCollisionShape* _shape;
    btStridingMeshInterface* _meshInterface;

    union
    {
        /** @script{ignore} */
        MeshData* meshData;
        /** @script{ignore} */
        HeightfieldData* heightfieldData;
    } _shapeData;
};

} // namespace tractor

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

#include "graphics/Drawable.h"
#include "graphics/Mesh.h"
#include "graphics/MeshSkin.h"
#include "renderer/Material.h"

namespace tractor
{

class Bundle;
class MeshSkin;
class Model;

/** Shared pointer type for Model. */
using ModelPtr = std::shared_ptr<Model>;

/** Weak pointer type for Model. */
using ModelWeakPtr = std::weak_ptr<Model>;

/**
 * Defines a Model or mesh renderer which is an instance of a Mesh.
 *
 * A model has a mesh that can be drawn with the specified materials for
 * each of the mesh parts within it.
 */
class Model : public Drawable
{
    friend class Node;
    friend class Scene;
    friend class Mesh;
    friend class Bundle;

  public:
    /**
     * Creates a new Model and returns ownership via shared_ptr.
     * @script{create}
     */
    static ModelPtr create(std::shared_ptr<Mesh> mesh);

    /**
     * Creates a new Model and returns a raw pointer.
     * The caller is responsible for deleting the Model.
     * 
     * @deprecated Prefer using create() which returns a shared_ptr for safer memory management.
     */
    static Model* createRaw(std::shared_ptr<Mesh> mesh);

    /**
     * Constructor.
     * 
     * @param mesh The mesh for this model.
     */
    explicit Model(std::shared_ptr<Mesh> mesh);

    /**
     * Destructor.
     */
    ~Model() = default;

    /**
     * Returns the Mesh for this Model.
     *
     * @return The Mesh for this Model.
     */
    Mesh* getMesh() const noexcept { return _mesh.get(); }

    /**
     * Returns the number of parts in the Mesh for this Model.
     *
     * @return The number of parts in the Mesh for this Model.
     */
    unsigned int getMeshPartCount() const;

    /**
     * Returns the Material currently bound to the specified mesh part.
     *
     * If partIndex is >= 0 and no Material is directly bound to the specified
     * mesh part, the shared Material will be returned.
     *
     * @param partIndex The index of the mesh part whose Material to return (-1 for shared material).
     *
     * @return The requested Material, or nullptr if no Material is set.
     */
    Material* getMaterial(int partIndex = -1);

    /**
     * Sets a material to be used for drawing this Model.
     *
     * The specified Material is applied for the MeshPart at the given index in
     * this Model's Mesh. A partIndex of -1 sets a shared Material for
     * all mesh parts, whereas a value of 0 or greater sets the Material for the
     * specified mesh part only.
     *
     * Mesh parts will use an explicitly set part material, if set; otherwise they
     * will use the globally set material.
     *
     * @param material The new material.
     * @param partIndex The index of the mesh part to set the material for (-1 for shared material).
     */
    void setMaterial(MaterialPtr material, int partIndex = -1);

    /**
     * Sets a material to be used for drawing this Model.
     *
     * A Material is created from the given vertex and fragment shader source files.
     * The Material is applied for the MeshPart at the given index in this Model's
     * Mesh. A partIndex of -1 sets a shared Material for all mesh parts, whereas a
     * value of 0 or greater sets the Material for the specified mesh part only.
     *
     * Mesh parts will use an explicitly set part material, if set; otherwise they
     * will use the globally set material.
     *
     * @param vshPath The path to the vertex shader file.
     * @param fshPath The path to the fragment shader file.
     * @param defines A new-line delimited list of preprocessor defines. May be nullptr.
     * @param partIndex The index of the mesh part to set the material for (-1 for shared material).
     *
     * @return The newly created and bound Material, or nullptr if the Material could not be created.
     */
    Material* setMaterial(const std::string& vshPath,
                          const std::string& fshPath,
                          const std::string& defines = EMPTY_STRING,
                          int partIndex = -1);

    /**
     * Sets a material to be used for drawing this Model.
     *
     * A Material is created from the specified material file.
     * The Material is applied for the MeshPart at the given index in this Model's
     * Mesh. A partIndex of -1 sets a shared Material for all mesh parts, whereas a
     * value of 0 or greater sets the Material for the specified mesh part only.
     *
     * Mesh parts will use an explicitly set part material, if set; otherwise they
     * will use the globally set material.
     *
     * @param materialPath The path to the material file.
     * @param partIndex The index of the mesh part to set the material for (-1 for shared material).
     *
     * @return The newly created and bound Material, or nullptr if the Material could not be created.
     */
    Material* setMaterial(const std::string& materialPath, int partIndex = -1);

    /**
     * Determines if a custom (non-shared) material is set for the specified part index.
     *
     * @param partIndex MeshPart index.
     *
     * @return True if a custom MeshPart material is set for the specified index, false otherwise.
     */
    bool hasMaterial(unsigned int partIndex) const;

    /**
     * Returns the MeshSkin.
     *
     * @return The MeshSkin, or nullptr if one is not set.
     */
    MeshSkin* getSkin() const noexcept { return _skin.get(); }

    /**
     * @see Drawable::draw
     *
     * Binds the vertex buffer and index buffers for the Mesh and
     * all of its MeshPart's and draws the mesh geometry.
     * Any other state necessary to render the Mesh, such as
     * rendering states, shader state, and so on, should be set
     * up before calling this method.
     */
    unsigned int draw(bool wireframe = false) override;

  private:
    /**
     * Default constructor.
     */
    Model() = default;

    /**
     * Hidden copy assignment operator.
     */
    Model& operator=(const Model&) = delete;

    /**
     * @see Drawable::setNode
     */
    void setNode(Node* node) override;

    /**
     * @see Drawable::clone
     */
    Drawable* clone(NodeCloneContext& context) override;

    /**
     * Sets the MeshSkin for this model.
     *
     * @param skin The MeshSkin for this model.
     */
    void setSkin(MeshSkin* skin);

    /**
     * Sets the specified material's node binding to this model's node.
     */
    void setMaterialNodeBinding(Material* m);

    void validatePartCount();

    std::shared_ptr<Mesh> _mesh;
    MaterialPtr _material;
    unsigned int _partCount{ 0 };
    std::unique_ptr<MaterialPtr[]> _partMaterials;
    std::unique_ptr<MeshSkin> _skin;
};

} // namespace tractor

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

#include "scene/Node.h"

namespace tractor
{

class MeshSkin;
class Bundle;

/**
 * Defines a joint node.
 *
 * This represent a joint in a skeleton that is hierarchially part of
 * a MeshSkin. This allows the vertices in the mesh to be blended and
 * animated using the sum of the blend weight that must add up to 1.0.
 */
class Joint : public Node
{
    friend class Node;
    friend class MeshSkin;
    friend class Bundle;

  public:
    /**
     * @see Node::getType()
     */
    Node::Type getType() const noexcept { return Node::JOINT; }

    /**
     * @see Node::getScene()
     */
    Scene* getScene() const;

    /**
     * Extends ScriptTarget::getTypeName() to return the type name of this class.
     *
     * @return The type name of this class: "Joint"
     * @see ScriptTarget::getTypeName()
     */
    const std::string& getTypeName() const;

    /**
     * Returns the inverse bind pose matrix for this joint.
     *
     * @return Inverse bind pose matrix.
     */
    const Matrix& getInverseBindPose() const noexcept { return _bindPose; }

  protected:
    /**
     * Constructor.
     */
    Joint(const std::string& id);

    /**
     * Destructor.
     */
    virtual ~Joint() = default;

    /**
     * Creates a new joint with the given id.
     *
     * @param id ID string.
     *
     * @return Newly created joint.
     */
    static Joint* create(const std::string& id);

    /**
     * Clones a single node and its data but not its children.
     * This method returns a node pointer but actually creates a Joint.
     *
     * @param context The clone context.
     *
     * @return Pointer to the newly created joint.
     */
    virtual Node* cloneSingleNode(NodeCloneContext& context) const;

    /**
     * Sets the inverse bind pose matrix.
     *
     * @param m Matrix representing the inverse bind pose for this Joint.
     */
    void setInverseBindPose(const Matrix& m);

    /**
     * Updates the joint matrix.
     *
     * @param bindShape The bind shape matrix.
     * @param matrixPalette The matrix palette to update.
     */
    void updateJointMatrix(const Matrix& bindShape, Vector4* matrixPalette);

    /**
     * Called when this Joint's transform changes.
     */
    void transformChanged();

  private:
    /**
     * Internal structure to track mesh skins referencing a joint.
     */
    struct SkinReference
    {
        MeshSkin* skin{ nullptr };
        SkinReference* next{ nullptr };

        SkinReference() = default;
        ~SkinReference();
    };

    /**
     * Constructor.
     */
    Joint(const Joint& copy);

    /**
     * Hidden copy assignment operator.
     */
    Joint& operator=(const Joint&) = delete;

    void addSkin(MeshSkin* skin);

    void removeSkin(MeshSkin* skin);

    /**
     * The Matrix representation of the Joint's bind pose.
     */
    Matrix _bindPose;

    /**
     * Flag used to mark if the Joint's matrix is dirty.
     */
    bool _jointMatrixDirty{ true };

    /**
     * Linked list of mesh skins that are referenced by this joint.
     */
    SkinReference _skin;
};

} // namespace tractor

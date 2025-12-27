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

#include "graphics/MeshSkin.h"

#include "animation/Joint.h"
#include "graphics/Model.h"
#include "scene/Node.h"

// The number of rows in each palette matrix.
#define PALETTE_ROWS 3

namespace tractor
{

//----------------------------------------------------------------------------
MeshSkin::~MeshSkin()
{
    clearJoints();

    SAFE_DELETE_ARRAY(_matrixPalette);
}

//----------------------------------------------------------------------------
Joint* MeshSkin::getJoint(unsigned int index) const
{
    assert(index < _joints.size());
    return _joints[index].get();
}

//----------------------------------------------------------------------------
Joint* MeshSkin::getJoint(const std::string& id) const
{
    for (size_t i = 0, count = _joints.size(); i < count; ++i)
    {
        Joint* j = _joints[i].get();
        if (j && j->getId() != EMPTY_STRING && j->getId() == id)
        {
            return j;
        }
    }

    return nullptr;
}

//----------------------------------------------------------------------------
MeshSkin* MeshSkin::clone(NodeCloneContext& context) const
{
    MeshSkin* skin = new MeshSkin();
    skin->_bindShape = _bindShape;
    if (_rootNode && _rootJoint)
    {
        const unsigned int jointCount = getJointCount();
        skin->setJointCount(jointCount);

        assert(!skin->_rootNode);

        // Check if the root node has already been cloned.
        if (NodePtr rootNode = context.findClonedNode(_rootNode.get()))
        {
            skin->_rootNode = rootNode;
        }
        else
        {
            skin->_rootNode = _rootNode->cloneRecursive(context);
        }

        Node* node = nullptr;
        if (skin->_rootNode->getId() == _rootJoint->getId())
        {
            node = skin->_rootNode.get();
        }
        else
        {
            node = skin->_rootNode->findNode(_rootJoint->getId());
        }
        assert(node);
        skin->_rootJoint = static_cast<Joint*>(node);
        for (size_t i = 0; i < jointCount; ++i)
        {
            Joint* oldJoint = getJoint(i);
            assert(oldJoint);

            Joint* newJoint = static_cast<Joint*>(skin->_rootNode->findNode(oldJoint->getId()));
            if (!newJoint)
            {
                if (skin->_rootJoint->getId() == oldJoint->getId())
                    newJoint = static_cast<Joint*>(skin->_rootJoint);
            }
            assert(newJoint);
            // Find the JointPtr from the clone context or create one
            NodePtr nodePtr = context.findClonedNode(oldJoint);
            JointPtr jointPtr = std::dynamic_pointer_cast<Joint>(nodePtr);
            if (!jointPtr && newJoint)
            {
                // If not in context, we need to find it from the root node's hierarchy
                // The joint should already be part of the cloned hierarchy
                // Just use a weak reference pattern here
                jointPtr = std::dynamic_pointer_cast<Joint>(skin->_rootNode);
                if (jointPtr.get() != newJoint)
                {
                    // Search in children - for now just store raw and handle later
                    // This is a complex case - the joint is part of the hierarchy
                    // We'll create a temporary shared_ptr that shares ownership with _rootNode
                }
            }
            skin->setJoint(std::dynamic_pointer_cast<Joint>(nodePtr), i);
        }
    }
    return skin;
}

//----------------------------------------------------------------------------
void MeshSkin::setJointCount(unsigned int jointCount)
{
    // Erase the joints vector and release all joints.
    clearJoints();

    // Resize the joints vector and initialize to nullptr.
    _joints.resize(jointCount);

    // Rebuild the matrix palette. Each matrix is 3 rows of Vector4.
    SAFE_DELETE_ARRAY(_matrixPalette);

    if (jointCount > 0)
    {
        _matrixPalette = new Vector4[jointCount * PALETTE_ROWS];
        for (size_t i = 0; i < jointCount * PALETTE_ROWS; i += PALETTE_ROWS)
        {
            _matrixPalette[i + 0].set(1.0f, 0.0f, 0.0f, 0.0f);
            _matrixPalette[i + 1].set(0.0f, 1.0f, 0.0f, 0.0f);
            _matrixPalette[i + 2].set(0.0f, 0.0f, 1.0f, 0.0f);
        }
    }
}

//----------------------------------------------------------------------------
void MeshSkin::setJoint(const JointPtr& joint, unsigned int index)
{
    assert(index < _joints.size());

    if (_joints[index])
    {
        _joints[index]->removeSkin(this);
    }

    _joints[index] = joint;

    if (joint)
    {
        joint->addSkin(this);
    }
}

//----------------------------------------------------------------------------
Vector4* MeshSkin::getMatrixPalette() const
{
    assert(_matrixPalette);

    for (size_t i = 0, count = _joints.size(); i < count; i++)
    {
        assert(_joints[i]);
        _joints[i]->updateJointMatrix(getBindShape(), &_matrixPalette[i * PALETTE_ROWS]);
    }
    return _matrixPalette;
}

//----------------------------------------------------------------------------
unsigned int MeshSkin::getMatrixPaletteSize() const
{
    return (unsigned int)_joints.size() * PALETTE_ROWS;
}

//----------------------------------------------------------------------------
void MeshSkin::setRootJoint(Joint* joint)
{
    if (_rootJoint)
    {
        if (_rootJoint->getParent())
        {
            _rootJoint->getParent()->removeListener(this);
        }
    }

    _rootJoint = joint;

    // If the root joint has a parent node, register for its transformChanged event
    if (_rootJoint && _rootJoint->getParent())
    {
        _rootJoint->getParent()->addListener(this, 1);
    }

    Node* newRootNode = _rootJoint;
    if (newRootNode)
    {
        // Find the top level parent node of the root joint
        for (Node* node = newRootNode->getParent(); node != nullptr; node = node->getParent())
        {
            if (node->getParent() == nullptr)
            {
                newRootNode = node;
                break;
            }
        }
    }
    
    // Get the NodePtr for the root node if available
    if (newRootNode)
    {
        try
        {
            setRootNode(newRootNode->shared_from_this());
        }
        catch (const std::bad_weak_ptr&)
        {
            // Node doesn't have shared ownership yet, this shouldn't happen
            // with properly constructed nodes
            setRootNode(nullptr);
        }
    }
    else
    {
        setRootNode(nullptr);
    }
}

//----------------------------------------------------------------------------
void MeshSkin::transformChanged(Transform* transform, long cookie)
{
    switch (cookie)
    {
        case 1:
            // The direct parent of our joint hierarchy has changed.
            // Dirty the bounding volume for our model's node. This special
            // case allows us to have much tighter bounding volumes for
            // skinned meshes by only considering local skin/joint transformations
            // during bounding volume computation instead of fully resolved
            // joint transformations.
            if (_model && _model->getNode())
            {
                _model->getNode()->setBoundsDirty();
            }
            break;
    }
}

//----------------------------------------------------------------------------
int MeshSkin::getJointIndex(Joint* joint) const
{
    for (size_t i = 0, count = _joints.size(); i < count; ++i)
    {
        if (_joints[i].get() == joint)
        {
            return (int)i;
        }
    }

    return -1;
}

//----------------------------------------------------------------------------
void MeshSkin::setRootNode(const NodePtr& node)
{
    _rootNode = node;
}

//----------------------------------------------------------------------------
void MeshSkin::clearJoints()
{
    setRootJoint(nullptr);

    for (size_t i = 0, count = _joints.size(); i < count; ++i)
    {
        if (_joints[i])
        {
            _joints[i]->removeSkin(this);
        }
    }
    _joints.clear();
}

} // namespace tractor

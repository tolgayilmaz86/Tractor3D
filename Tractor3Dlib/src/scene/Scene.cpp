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

#include "scene/Scene.h"

#include "animation/Joint.h"
#include "audio/AudioListener.h"
#include "graphics/MeshSkin.h"
#include "graphics/Terrain.h"
#include "scene/Bundle.h"
#include "scene/SceneLoader.h"
#include "utils/StringUtil.h"

namespace tractor
{

// Global list of active scenes (raw pointers for lookup only, not owning)
static std::vector<Scene*> __sceneList;

//----------------------------------------------------------------------------
Scene::Scene() { __sceneList.push_back(this); }

//----------------------------------------------------------------------------
Scene::~Scene()
{
    // Unbind our active camera from the audio listener
    if (_activeCamera)
    {
        AudioListener* audioListener = AudioListener::getInstance();
        if (audioListener && (audioListener->getCamera() == _activeCamera.get()))
        {
            audioListener->setCamera(nullptr);
        }
    }

    // Remove all nodes from the scene
    removeAllNodes();

    // Remove the scene from global list
    std::vector<Scene*>::iterator itr = std::find(__sceneList.begin(), __sceneList.end(), this);
    if (itr != __sceneList.end()) __sceneList.erase(itr);
}

//----------------------------------------------------------------------------
ScenePtr Scene::create(const std::string& id)
{
    auto scene = std::make_shared<Scene>();
    scene->setId(id);
    return scene;
}

//----------------------------------------------------------------------------
ScenePtr Scene::load(const std::string& filePath)
{
    if (endsWithIgnoreCase(filePath, ".gpb"))
    {
        ScenePtr scene = nullptr;
        BundlePtr bundle = Bundle::create(filePath);
        if (bundle)
        {
            scene = bundle->loadScene();
        }
        return scene;
    }
    return SceneLoader::load(filePath);
}

//----------------------------------------------------------------------------
Scene* Scene::getScene(const std::string& id)
{
    if (id.empty()) return __sceneList.size() ? __sceneList[0] : nullptr;

    for (size_t i = 0, count = __sceneList.size(); i < count; ++i)
    {
        if (__sceneList[i]->_id == id) return __sceneList[i];
    }

    return nullptr;
}

//----------------------------------------------------------------------------
Node* Scene::findNode(const std::string& id, bool recursive, bool exactMatch) const
{
    // Search immediate children first.
    for (Node* child = getFirstNode(); child != nullptr; child = child->getNextSibling())
    {
        // Does this child's ID match?
        if ((exactMatch && child->_id == id) || (!exactMatch && child->_id.find(id) == 0))
        {
            return child;
        }
    }

    // Recurse.
    if (recursive)
    {
        for (Node* child = getFirstNode(); child != nullptr; child = child->getNextSibling())
        {
            Node* match = child->findNode(id, true, exactMatch);
            if (match)
            {
                return match;
            }
        }
    }
    return nullptr;
}

//----------------------------------------------------------------------------
unsigned int Scene::findNodes(const std::string& id,
                              std::vector<Node*>& nodes,
                              bool recursive,
                              bool exactMatch) const
{
    unsigned int count = 0;

    // Search immediate children first.
    for (Node* child = getFirstNode(); child != nullptr; child = child->getNextSibling())
    {
        // Does this child's ID match?
        if ((exactMatch && child->_id == id) || (!exactMatch && child->_id.find(id) == 0))
        {
            nodes.push_back(child);
            ++count;
        }
    }

    // Recurse.
    if (recursive)
    {
        for (Node* child = getFirstNode(); child != nullptr; child = child->getNextSibling())
        {
            count += child->findNodes(id, nodes, true, exactMatch);
        }
    }

    return count;
}

//----------------------------------------------------------------------------
void Scene::visitNode(Node* node, const char* visitMethod)
{
    ScriptController* sc = Game::getInstance()->getScriptController();

    // Invoke the visit method for this node.
    bool result;
    if (!sc->executeFunction<bool>(visitMethod, "<Node>", &result, (void*)node) || !result) return;

    // If this node has a model with a mesh skin, visit the joint hierarchy within it
    // since we don't add joint hierarcies directly to the scene. If joints are never
    // visited, it's possible that nodes embedded within the joint hierarchy that contain
    // models will never get visited (and therefore never get drawn).
    Model* model = dynamic_cast<Model*>(node->getDrawable());
    if (model && model->_skin && model->_skin->_rootNode)
    {
        visitNode(model->_skin->_rootNode.get(), visitMethod);
    }

    // Recurse for all children.
    for (Node* child = node->getFirstChild(); child != nullptr; child = child->getNextSibling())
    {
        visitNode(child, visitMethod);
    }
}

//----------------------------------------------------------------------------
NodePtr Scene::addNode(const std::string& id)
{
    NodePtr node = Node::create(id);
    assert(node);
    addNode(node);
    return node;
}

//----------------------------------------------------------------------------
void Scene::addNode(const NodePtr& node)
{
    assert(node);

    if (node->_scene == this)
    {
        // The node is already a member of this scene.
        return;
    }

    // If the node is part of another scene, remove it.
    if (node->_scene && node->_scene != this)
    {
        node->_scene->removeNode(node.get());
    }

    // If the node is part of another node hierarchy, remove it.
    if (node->getParent())
    {
        node->getParent()->removeChild(node.get());
    }

    // Link the new node into the end of our list.
    if (_lastNode)
    {
        _lastNode->_nextSibling = node;
        node->_prevSibling = _lastNode.get();
        _lastNode = node;
    }
    else
    {
        _firstNode = _lastNode = node;
    }

    node->_scene = this;

    ++_nodeCount;

    // If we don't have an active camera set, then check for one and set it.
    if (_activeCamera == nullptr)
    {
        Camera* camera = node->getCamera();
        if (camera)
        {
            setActiveCamera(camera);
        }
    }
}

//----------------------------------------------------------------------------
void Scene::removeNode(Node* node)
{
    assert(node);

    if (node->_scene != this) return;

    // Find the shared_ptr for this node
    NodePtr nodePtr;
    if (_firstNode.get() == node)
    {
        nodePtr = _firstNode;
        _firstNode = node->_nextSibling;
    }
    else
    {
        for (NodePtr n = _firstNode; n != nullptr; n = n->_nextSibling)
        {
            if (n->_nextSibling.get() == node)
            {
                nodePtr = n->_nextSibling;
                break;
            }
        }
    }
    
    if (_lastNode.get() == node)
    {
        // Find new last node
        if (_firstNode)
        {
            NodePtr n = _firstNode;
            while (n->_nextSibling && n->_nextSibling.get() != node)
                n = n->_nextSibling;
            _lastNode = n;
        }
        else
        {
            _lastNode.reset();
        }
    }

    node->remove();
    node->_scene = nullptr;

    --_nodeCount;
    // nodePtr will be released when it goes out of scope
}

//----------------------------------------------------------------------------
void Scene::removeAllNodes()
{
    while (_firstNode)
    {
        removeNode(_firstNode.get());
    }
}

//----------------------------------------------------------------------------
void Scene::setActiveCamera(Camera* camera)
{
    // Make sure we don't release the camera if the same camera is set twice.
    if (_activeCamera.get() != camera)
    {
        AudioListener* audioListener = AudioListener::getInstance();

        if (_activeCamera)
        {
            // Unbind the active camera from the audio listener
            if (audioListener && (audioListener->getCamera() == _activeCamera.get()))
            {
                audioListener->setCamera(nullptr);
            }
        }

        // Find the node that owns this camera and get the shared_ptr
        if (camera && camera->getNode())
        {
            _activeCamera = camera->getNode()->_camera;
        }
        else
        {
            _activeCamera.reset();
        }

        if (_activeCamera)
        {
            if (audioListener && _bindAudioListenerToCamera)
            {
                audioListener->setCamera(_activeCamera.get());
            }
        }
    }
}

//----------------------------------------------------------------------------
void Scene::bindAudioListenerToCamera(bool bind)
{
    if (_bindAudioListenerToCamera != bind)
    {
        _bindAudioListenerToCamera = bind;

        if (AudioListener::getInstance())
        {
            AudioListener::getInstance()->setCamera(bind ? _activeCamera.get() : nullptr);
        }
    }
}

//----------------------------------------------------------------------------
void Scene::setAmbientColor(float red, float green, float blue)
{
    _ambientColor.set(red, green, blue);
}

//----------------------------------------------------------------------------
void Scene::update(float elapsedTime)
{
    for (Node* node = getFirstNode(); node != nullptr; node = node->getNextSibling())
    {
        if (node->isEnabled()) node->update(elapsedTime);
    }
}

//----------------------------------------------------------------------------
void Scene::reset()
{
    _nextItr = nullptr;
    _nextReset = true;
}

//----------------------------------------------------------------------------
Node* Scene::getNext()
{
    if (_nextReset)
    {
        _nextItr = findNextVisibleSibling(getFirstNode());
        _nextReset = false;
    }
    else if (_nextItr)
    {
        Node* node = findNextVisibleSibling(_nextItr->getFirstChild());
        if (node == nullptr)
        {
            node = findNextVisibleSibling(_nextItr->getNextSibling());
            if (node == nullptr)
            {
                // Find first parent with a sibling
                node = _nextItr->getParent();
                while (node && (!findNextVisibleSibling(node->getNextSibling())))
                {
                    node = node->getParent();
                }
                if (node)
                {
                    node = findNextVisibleSibling(node->getNextSibling());
                }
            }
        }
        _nextItr = node;
    }
    return _nextItr;
}

//----------------------------------------------------------------------------
Node* Scene::findNextVisibleSibling(Node* node)
{
    while (node != nullptr && !isNodeVisible(node))
    {
        node = node->getNextSibling();
    }
    return node;
}

//----------------------------------------------------------------------------
bool Scene::isNodeVisible(Node* node)
{
    if (!node->isEnabled()) return false;

    if (node->getDrawable() || node->getLight() || node->getCamera())
    {
        return true;
    }
    else
    {
        return node->getBoundingSphere().intersects(_activeCamera->getFrustum());
    }
}

} // namespace tractor

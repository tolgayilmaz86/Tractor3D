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

#include "scene/SceneLoader.h"

#include "audio/AudioSource.h"
#include "framework/Game.h"
#include "graphics/Light.h"
#include "graphics/ParticleEmitter.h"
#include "graphics/Sprite.h"
#include "graphics/Terrain.h"
#include "graphics/TileSet.h"
#include "renderer/Text.h"
#include "scene/Bundle.h"

namespace tractor
{

//----------------------------------------------------------------------------
// Utility functions (shared with Properties).
extern void calculateNamespacePath(const std::string& urlString,
                                   std::string& fileString,
                                   std::vector<std::string>& namespacePath);

//----------------------------------------------------------------------------
extern Properties* getPropertiesFromNamespacePath(Properties* properties,
                                                  const std::vector<std::string>& namespacePath);

//----------------------------------------------------------------------------
Scene* SceneLoader::load(const std::string& url)
{
    SceneLoader loader;
    return loader.loadInternal(url);
}

//----------------------------------------------------------------------------
Scene* SceneLoader::loadInternal(const std::string& url)
{
    // Get the file part of the url that we are loading the scene from.
    std::string id;
    splitURL(url, &_path, &id);

    // Load the scene properties from file.
    auto properties = std::unique_ptr<Properties>(Properties::create(url));
    if (properties == nullptr)
    {
        GP_ERROR("Failed to load scene file '%s'.", url);
        return nullptr;
    }

    // Check if the properties object is valid and has a valid namespace.
    Properties* sceneProperties =
        !properties->getNamespace().empty() ? properties.get() : properties->getNextNamespace();
    if (!sceneProperties || sceneProperties->getNamespace() != "scene")
    {
        GP_ERROR("Failed to load scene from properties object: must be non-null object and have "
                 "namespace equal to 'scene'.");
        return nullptr;
    }

    // Get the path to the main GPB.
    std::string path;
    if (sceneProperties->getPath("path", path))
    {
        _gpbPath = path;
    }

    // Build the node URL/property and animation reference tables and load the referenced
    // files/store the inline properties objects.
    buildReferenceTables(sceneProperties);
    loadReferencedFiles();

    // Load the main scene data from GPB and apply the global scene properties.
    if (!_gpbPath.empty())
    {
        // Load scene from bundle
        _scene = loadMainSceneData(sceneProperties);
        if (!_scene)
        {
            GP_WARN("Failed to load main scene from bundle.");
            return nullptr;
        }
    }
    else
    {
        // Create a new empty scene
        _scene = Scene::create(sceneProperties->getId());
    }

    // First apply the node url properties. Following that,
    // apply the normal node properties and create the animations.
    // We apply physics properties after all other node properties
    // so that the transform (SRT) properties get applied before
    // processing physics collision objects.
    applyNodeUrls();
    applyNodeProperties(sceneProperties,
                        SceneNodeProperty::AUDIO | SceneNodeProperty::MATERIAL
                            | SceneNodeProperty::PARTICLE | SceneNodeProperty::TERRAIN
                            | SceneNodeProperty::LIGHT | SceneNodeProperty::CAMERA
                            | SceneNodeProperty::ROTATE | SceneNodeProperty::SCALE
                            | SceneNodeProperty::TRANSLATE | SceneNodeProperty::SCRIPT
                            | SceneNodeProperty::SPRITE | SceneNodeProperty::TILESET
                            | SceneNodeProperty::TEXT | SceneNodeProperty::ENABLED);
    applyNodeProperties(sceneProperties, SceneNodeProperty::COLLISION_OBJECT);

    // Apply node tags
    for (size_t i = 0, sncount = _sceneNodes.size(); i < sncount; ++i)
    {
        applyTags(_sceneNodes[i]);
    }

    // Set active camera
    auto activeCamera = sceneProperties->getString("activeCamera");
    if (!activeCamera.empty())
    {
        Node* camera = _scene->findNode(activeCamera);
        if (camera && camera->getCamera()) _scene->setActiveCamera(camera->getCamera());
    }

    // Set ambient and light properties
    Vector3 vec3;
    if (sceneProperties->getVector3("ambientColor", &vec3))
        _scene->setAmbientColor(vec3.x, vec3.y, vec3.z);

    // Create animations for scene
    createAnimations();

    // Find the physics properties object.
    Properties* physics = nullptr;
    sceneProperties->rewind();
    while (true)
    {
        Properties* ns = sceneProperties->getNextNamespace();
        if (ns == nullptr || ns->getNamespace() == "physics")
        {
            physics = ns;
            break;
        }
    }

    // Load physics properties and constraints.
    if (physics) loadPhysics(physics);

    std::erase_if(_propertiesFromFile,
                  [](auto& pair)
                  {
                      SAFE_DELETE(pair.second);
                      return true; // erase each of them
                  });

    return _scene;
}

//----------------------------------------------------------------------------
void SceneLoader::applyTags(SceneNode& sceneNode)
{
    // Apply tags for this scene node
    // Type of std::map<std::string, std::string>
    for (const auto& tag : sceneNode._tags)
    {
        for (const auto& node : sceneNode._nodes)
        {
            node->setTag(tag.first, tag.second);
        }
    }

    // Process children
    for (auto& node : sceneNode._children)
    {
        applyTags(node);
    }
}

//----------------------------------------------------------------------------
void SceneLoader::addSceneAnimation(const std::string& animationID,
                                    const std::string& targetID,
                                    const std::string& url)
{
    // If there is a file that needs to be loaded later, add an
    // empty entry to the properties table to signify it.
    if (url.length() > 0 && _properties.count(url) == 0) _properties[url] = nullptr;

    // Add the animation to the list of animations to be resolved later.
    _animations.emplace_back(SceneAnimation(animationID, targetID, url));
}

//----------------------------------------------------------------------------
void SceneLoader::addSceneNodeProperty(SceneNode& sceneNode,
                                       SceneNodeProperty::Type type,
                                       const std::string& value,
                                       bool supportsUrl,
                                       int index)
{
    bool isUrl = false;

    if (supportsUrl)
    {
        // If there is a non-GPB file that needs to be loaded later, add an
        // empty entry to the properties table to signify it.
        if (value.length() > 0 && value.find(".") != std::string::npos
            && value.find(".gpb") == std::string::npos && _properties.count(value) == 0)
        {
            isUrl = true;
            _properties[value] = nullptr;
        }
    }

    SceneNodeProperty prop(type, value, index, isUrl);

    // Parse for wildcharacter character (only supported on the URL attribute)
    if (type & SceneNodeProperty::URL)
    {
        if (value.length() > 1 && value.at(value.length() - 1) == '*')
        {
            prop._value = value.substr(0, value.length() - 1);
            sceneNode._exactMatch = false;
        }
    }

    // Add the node property to the list of node properties to be resolved later.
    sceneNode._properties.push_back(prop);
}

//----------------------------------------------------------------------------
void SceneLoader::applyNodeProperties(const Properties* sceneProperties, size_t typeFlags)
{
    for (auto& node : _sceneNodes)
    {
        applyNodeProperties(node, sceneProperties, typeFlags);
    }
}

//----------------------------------------------------------------------------
void SceneLoader::applyNodeProperties(SceneNode& sceneNode,
                                      const Properties* sceneProperties,
                                      size_t typeFlags)
{
    // Apply properties for this node
    for (size_t i = 0, pcount = sceneNode._properties.size(); i < pcount; ++i)
    {
        SceneNodeProperty& snp = sceneNode._properties[i];
        if (typeFlags & snp._type)
        {
            for (auto& node : sceneNode._nodes)
                applyNodeProperty(sceneNode, node, sceneProperties, snp);
        }
    }

    // Apply properties to child nodes
    for (auto& child : sceneNode._children)
    {
        applyNodeProperties(child, sceneProperties, typeFlags);
    }
}

//----------------------------------------------------------------------------
void SceneLoader::applyNodeProperty(SceneNode& sceneNode,
                                    Node* node,
                                    const Properties* sceneProperties,
                                    const SceneNodeProperty& snp)
{
    if (snp._type & SceneNodeProperty::AUDIO || snp._type & SceneNodeProperty::MATERIAL
        || snp._type & SceneNodeProperty::PARTICLE || snp._type & SceneNodeProperty::TERRAIN
        || snp._type & SceneNodeProperty::LIGHT || snp._type & SceneNodeProperty::CAMERA
        || snp._type & SceneNodeProperty::COLLISION_OBJECT || snp._type & SceneNodeProperty::SPRITE
        || snp._type & SceneNodeProperty::TILESET || snp._type & SceneNodeProperty::TEXT)
    {
        // Check to make sure the referenced properties object was loaded properly.
        Properties* p = _properties[snp._value];
        if (!p)
        {
            GP_ERROR("The referenced node data at url '%s' failed to load.", snp._value.c_str());
            return;
        }
        p->rewind();

        // If the URL didn't specify a particular namespace within the file, pick the first one.
        p = p->getNamespace().length() > 0 ? p : p->getNextNamespace();

        switch (snp._type)
        {
            case SceneNodeProperty::AUDIO:
            {
                AudioSource* audioSource = AudioSource::create(p);
                node->setAudioSource(audioSource);
                SAFE_RELEASE(audioSource);
                break;
            }
            case SceneNodeProperty::MATERIAL:
            {
                Model* model = dynamic_cast<Model*>(node->getDrawable());
                if (model)
                {
                    Material* material = Material::create(p);
                    model->setMaterial(material, snp._index);
                    SAFE_RELEASE(material);
                }
                else
                {
                    GP_ERROR("Attempting to set a material on node '%s', which has no model.",
                             sceneNode._nodeID);
                    return;
                }
                break;
            }
            case SceneNodeProperty::PARTICLE:
            {
                ParticleEmitter* particleEmitter = ParticleEmitter::create(p);
                node->setDrawable(particleEmitter);
                SAFE_RELEASE(particleEmitter);
                break;
            }
            case SceneNodeProperty::TERRAIN:
            {
                Terrain* terrain = Terrain::create(p);
                node->setDrawable(terrain);
                SAFE_RELEASE(terrain);
                break;
            }
            case SceneNodeProperty::LIGHT:
            {
                Light* light = Light::create(p);
                node->setLight(light);
                SAFE_RELEASE(light);
                break;
            }
            case SceneNodeProperty::CAMERA:
            {
                Camera* camera = Camera::create(p);
                node->setCamera(camera);
                SAFE_RELEASE(camera);
                break;
            }
            case SceneNodeProperty::COLLISION_OBJECT:
            {
                // Check to make sure the type of the namespace used to load the physics collision
                // object is correct.
                if (snp._type & SceneNodeProperty::COLLISION_OBJECT
                    && p->getNamespace() != "collisionObject")
                {
                    GP_ERROR("Attempting to set a physics collision object on a node using a '%s' "
                             "definition.",
                             p->getNamespace());
                    return;
                }
                else
                {
                    // If the scene file specifies a rigid body model, use it for creating the
                    // collision object.
                    Properties* np = sceneProperties->getNamespace(sceneNode._nodeID);
                    std::string name = np->getString("rigidBodyModel");

                    // Allow both property names
                    if (np && name.empty())
                    {
                        name = np->getString("collisionMesh");
                    }

                    if (!name.empty())
                    {
                        Node* modelNode = _scene->findNode(name);
                        if (!modelNode)
                        {
                            GP_ERROR("Node '%s' does not exist; attempting to use its model for "
                                     "collision object creation.",
                                     name);
                            return;
                        }
                        else
                        {
                            if (dynamic_cast<Model*>(modelNode->getDrawable()) == nullptr)
                            {
                                GP_ERROR("Node '%s' does not have a model; attempting to use its "
                                         "model for collision object creation.",
                                         name);
                            }
                            else
                            {
                                // Temporarily set rigidBody model on model so it's used during
                                // collision object creation.
                                Model* model = dynamic_cast<Model*>(node->getDrawable());

                                // Up ref count to prevent node from releasing the model when we
                                // swap it.
                                if (model) model->addRef();

                                // Create collision object with new rigidBodyModel (aka
                                // collisionMesh) set.
                                node->setDrawable(dynamic_cast<Model*>(modelNode->getDrawable()));
                                node->setCollisionObject(p);

                                // Restore original model.
                                node->setDrawable(model);

                                // Decrement temporarily added reference.
                                if (model) model->release();
                            }
                        }
                    }
                    else
                        node->setCollisionObject(p);
                }
                break;
            }
            case SceneNodeProperty::SPRITE:
            {
                Sprite* sprite = Sprite::create(p);
                node->setDrawable(sprite);
                SAFE_RELEASE(sprite);
                break;
            }
            case SceneNodeProperty::TILESET:
            {
                TileSet* tileset = TileSet::create(p);
                node->setDrawable(tileset);
                SAFE_RELEASE(tileset);
                break;
            }
            case SceneNodeProperty::TEXT:
            {
                Text* text = Text::create(p);
                node->setDrawable(text);
                SAFE_RELEASE(text);
                break;
            }
            default:
                GP_ERROR("Unsupported node property type (%d).", snp._type);
                break;
        }
    }
    else
    {
        // Handle simple types (scale, rotate, translate, script, etc)
        switch (snp._type)
        {
            case SceneNodeProperty::TRANSLATE:
            {
                Vector3 t;
                if (Properties::parseVector3(snp._value, &t)) node->translate(t);
                break;
            }
            case SceneNodeProperty::ROTATE:
            {
                Quaternion r;
                if (Properties::parseAxisAngle(snp._value, &r)) node->rotate(r);
                break;
            }
            case SceneNodeProperty::SCALE:
            {
                Vector3 s;
                if (Properties::parseVector3(snp._value, &s)) node->scale(s);
                break;
            }
            case SceneNodeProperty::SCRIPT:
                node->addScript(snp._value);
                break;
            case SceneNodeProperty::ENABLED:
                node->setEnabled(snp._value.compare("true") == 0);
                break;
            default:
                GP_ERROR("Unsupported node property type (%d).", snp._type);
                break;
        }
    }
}

//----------------------------------------------------------------------------
void SceneLoader::applyNodeUrls()
{
    // Apply all URL node properties so that when we go to apply
    // the other node properties, the node is in the scene.
    for (size_t i = 0, count = _sceneNodes.size(); i < count; ++i)
    {
        applyNodeUrls(_sceneNodes[i], nullptr);
    }
}

//----------------------------------------------------------------------------
void SceneLoader::applyNodeUrls(SceneNode& sceneNode, Node* parent)
{
    // Iterate backwards over the properties list so we can remove properties as we go
    // without danger of indexing out of bounds.
    bool hasURL = false;
    for (auto it = sceneNode._properties.rbegin(); it != sceneNode._properties.rend(); ++it)
    {
        const auto& snp = *it;
        if (snp._type != SceneNodeProperty::URL) continue; // skip nodes without urls

        hasURL = true;

        std::string file;
        std::string id;
        splitURL(snp._value, &file, &id);

        if (file.empty())
        {
            // The node is from the main GPB and should just be renamed.

            // TODO: Should we do all nodes with this case first to allow users to stitch in nodes
            // with IDs equal to IDs that were in the original GPB file but were changed in the
            // scene file?
            if (sceneNode._exactMatch)
            {
                processExactMatchNode(sceneNode, parent, id);
            }
            else
            {
                processPartialMatchNodes(sceneNode, parent, id);
            }
        }
        // An external file was referenced, so load the node(s) from file and then insert it into
        // the scene with the new ID.
        else
        {
            processExternalFile(sceneNode, parent, file, id);
        }

        // Convert reverse iterator to forward iterator for erase
        auto forward_it = std::next(it).base();
        // Remove the 'url' node property since we are done applying it.
        sceneNode._properties.erase(forward_it);

        // Processed URL property, no need to inspect remaining properties
        break;
    }

    if (!hasURL)
    {
        // No explicit URL, find the node in the main scene with the existing ID
        Node* node =
            parent ? parent->findNode(sceneNode._nodeID) : _scene->findNode(sceneNode._nodeID);
        if (node)
        {
            sceneNode._nodes.push_back(node);
        }
        else
        {
            // There is no node in the scene with this ID, so create a new empty node
            node = Node::create(sceneNode._nodeID);
            parent ? parent->addChild(node) : _scene->addNode(node);
            node->release();
            sceneNode._nodes.push_back(node);
        }
    }

    // Apply to child nodes
    for (auto& parent : sceneNode._nodes)
    {
        for (auto& child : sceneNode._children)
        {
            applyNodeUrls(child, parent);
        }
    }
}

//----------------------------------------------------------------------------
void SceneLoader::buildReferenceTables(Properties* sceneProperties)
{
    // Go through the child namespaces of the scene.
    sceneProperties->rewind();
    Properties* ns;
    while ((ns = sceneProperties->getNextNamespace()) != nullptr)
    {
        if (ns->getNamespace() == "node")
        {
            if (ns->getId().empty())
            {
                GP_ERROR("Attempting to load a node without an ID.");
                continue;
            }

            parseNode(ns, nullptr, _path + "#" + ns->getId() + "/");
        }
        else if (ns->getNamespace() == "animations")
        {
            // Load all the animations.
            Properties* animation;
            while ((animation = ns->getNextNamespace()) != nullptr)
            {
                if (animation->getNamespace() == "animation")
                {
                    auto animationID = animation->getId();
                    if (animationID.empty())
                    {
                        GP_ERROR("Attempting to load an animation without an ID.");
                        continue;
                    }

                    auto url = animation->getString("url");
                    if (url.empty())
                    {
                        GP_ERROR("Attempting to load animation '%s' without a URL.", animationID);
                        continue;
                    }
                    auto targetID = animation->getString("target");
                    if (targetID.empty())
                    {
                        GP_ERROR("Attempting to load animation '%s' without a target.", animationID);
                        continue;
                    }

                    addSceneAnimation(animationID, targetID, url);
                }
                else
                {
                    GP_ERROR("Unsupported child namespace (of 'animations'): %s", ns->getNamespace());
                }
            }
        }
        else if (ns->getNamespace() == "physics")
        {
            // Note: we don't load physics until the whole scene file has been
            // loaded so that all node references (i.e. for constraints) can be resolved.
        }
        else
        {
            // TODO: Should we ignore these items? They could be used for generic properties file
            // inheritance.
            GP_ERROR("Unsupported child namespace (of 'scene'): %s", ns->getNamespace());
        }
    }
}

//----------------------------------------------------------------------------
void SceneLoader::parseNode(Properties* ns, SceneNode* parent, const std::string& path)
{
    std::string propertyUrl;
    std::string name;

    // Add a SceneNode to the end of the list.
    std::vector<SceneNode>& list = parent ? parent->_children : _sceneNodes;
    list.emplace_back(SceneNode());
    SceneNode& sceneNode = list[list.size() - 1];
    sceneNode._nodeID = ns->getId();

    // Parse the node's sub-namespaces.
    Properties* subns;
    while ((subns = ns->getNextNamespace()) != nullptr)
    {
        if (subns->getNamespace() == "node")
        {
            parseNode(subns, &sceneNode, path + subns->getId() + "/");
        }
        else if (subns->getNamespace() == "audio")
        {
            propertyUrl = path + "audio/" + std::string(subns->getId());
            addSceneNodeProperty(sceneNode, SceneNodeProperty::AUDIO, propertyUrl);
            _properties[propertyUrl] = subns;
        }
        else if (subns->getNamespace() == "material")
        {
            propertyUrl = path + "material/" + std::string(subns->getId());
            addSceneNodeProperty(sceneNode, SceneNodeProperty::MATERIAL, propertyUrl);
            _properties[propertyUrl] = subns;
        }
        else if (subns->getNamespace() == "particle")
        {
            propertyUrl = path + "particle/" + std::string(subns->getId());
            addSceneNodeProperty(sceneNode, SceneNodeProperty::PARTICLE, propertyUrl);
            _properties[propertyUrl] = subns;
        }
        else if (subns->getNamespace() == "terrain")
        {
            propertyUrl = path + "terrain/" + std::string(subns->getId());
            addSceneNodeProperty(sceneNode, SceneNodeProperty::TERRAIN, propertyUrl);
            _properties[propertyUrl] = subns;
        }
        else if (subns->getNamespace() == "light")
        {
            propertyUrl = path + "light/" + std::string(subns->getId());
            addSceneNodeProperty(sceneNode, SceneNodeProperty::LIGHT, propertyUrl);
            _properties[propertyUrl] = subns;
        }
        else if (subns->getNamespace() == "camera")
        {
            propertyUrl = path + "camera/" + std::string(subns->getId());
            addSceneNodeProperty(sceneNode, SceneNodeProperty::CAMERA, propertyUrl);
            _properties[propertyUrl] = subns;
        }
        else if (subns->getNamespace() == "collisionObject")
        {
            propertyUrl = path + "collisionObject/" + std::string(subns->getId());
            addSceneNodeProperty(sceneNode, SceneNodeProperty::COLLISION_OBJECT, propertyUrl);
            _properties[propertyUrl] = subns;
        }
        else if (subns->getNamespace() == "sprite")
        {
            propertyUrl = path + "sprite/" + std::string(subns->getId());
            addSceneNodeProperty(sceneNode, SceneNodeProperty::SPRITE, propertyUrl);
            _properties[propertyUrl] = subns;
        }
        else if (subns->getNamespace() == "tileset")
        {
            propertyUrl = path + "tileset/" + std::string(subns->getId());
            addSceneNodeProperty(sceneNode, SceneNodeProperty::TILESET, propertyUrl);
            _properties[propertyUrl] = subns;
        }
        else if (subns->getNamespace() == "text")
        {
            propertyUrl = path + "text/" + std::string(subns->getId());
            addSceneNodeProperty(sceneNode, SceneNodeProperty::TEXT, propertyUrl);
            _properties[propertyUrl] = subns;
        }
        else if (subns->getNamespace() == "tags")
        {
            while (auto property = subns->getNextProperty())
            {
                sceneNode._tags[property->name] = subns->getString();
            }
        }
        else
        {
            GP_ERROR("Unsupported child namespace '%s' of 'node' namespace.", subns->getNamespace());
        }
    }

    // Parse the node's attributes.
    while (auto property = ns->getNextProperty())
    {
        auto name = property->name;
        if (name == "url")
        {
            addSceneNodeProperty(sceneNode, SceneNodeProperty::URL, ns->getString(), true);
        }
        else if (name == "audio")
        {
            addSceneNodeProperty(sceneNode, SceneNodeProperty::AUDIO, ns->getString(), true);
        }
        else if (name.rfind("material", 0) == 0)
        {
            int materialIndex = -1;
            auto bracketPos = name.find('[');
            if (bracketPos != std::string::npos && name.size() >= bracketPos + 3)
            {
                std::string indexString = name.substr(bracketPos + 1, name.size() - bracketPos - 2);
                materialIndex = std::stoi(indexString);
            }
            addSceneNodeProperty(sceneNode,
                                 SceneNodeProperty::MATERIAL,
                                 ns->getString(),
                                 true,
                                 materialIndex);
        }
        else if (name == "particle")
        {
            addSceneNodeProperty(sceneNode, SceneNodeProperty::PARTICLE, ns->getString(), true);
        }
        else if (name == "terrain")
        {
            addSceneNodeProperty(sceneNode, SceneNodeProperty::TERRAIN, ns->getString(), true);
        }
        else if (name == "light")
        {
            addSceneNodeProperty(sceneNode, SceneNodeProperty::LIGHT, ns->getString(), true);
        }
        else if (name == "camera")
        {
            addSceneNodeProperty(sceneNode, SceneNodeProperty::CAMERA, ns->getString(), true);
        }
        else if (name == "collisionObject")
        {
            addSceneNodeProperty(sceneNode, SceneNodeProperty::COLLISION_OBJECT, ns->getString(), true);
        }
        else if (name == "sprite")
        {
            addSceneNodeProperty(sceneNode, SceneNodeProperty::SPRITE, ns->getString(), true);
        }
        else if (name == "tileset")
        {
            addSceneNodeProperty(sceneNode, SceneNodeProperty::TILESET, ns->getString(), true);
        }
        else if (name == "text")
        {
            addSceneNodeProperty(sceneNode, SceneNodeProperty::TEXT, ns->getString(), true);
        }
        else if (name == "rigidBodyModel")
        {
            // Ignore this for now. We process this when we do rigid body creation.
        }
        else if (name == "collisionMesh")
        {
            // Ignore this for now (new alias for rigidBodyModel). We process this when we do rigid
            // body creation.
        }
        else if (name == "translate")
        {
            addSceneNodeProperty(sceneNode, SceneNodeProperty::TRANSLATE, ns->getString());
        }
        else if (name == "rotate")
        {
            addSceneNodeProperty(sceneNode, SceneNodeProperty::ROTATE, ns->getString());
        }
        else if (name == "scale")
        {
            addSceneNodeProperty(sceneNode, SceneNodeProperty::SCALE, ns->getString());
        }
        else if (name == "script")
        {
            addSceneNodeProperty(sceneNode, SceneNodeProperty::SCRIPT, ns->getString());
        }
        else if (name == "enabled")
        {
            addSceneNodeProperty(sceneNode, SceneNodeProperty::ENABLED, ns->getString());
        }
        else
        {
            GP_ERROR("Unsupported node property: %s = %s", name.c_str(), ns->getString());
        }
    }
}

//----------------------------------------------------------------------------
void SceneLoader::createAnimations()
{
    // Create the scene animations.
    for (size_t i = 0, count = _animations.size(); i < count; i++)
    {
        // If the target node doesn't exist in the scene, then we
        // can't do anything so we skip to the next animation.
        Node* node = _scene->findNode(_animations[i]._targetID);
        if (!node)
        {
            GP_ERROR("Attempting to create an animation targeting node '%s', which does not exist "
                     "in the scene.",
                     _animations[i]._targetID);
            continue;
        }

        // Check to make sure the referenced properties object was loaded properly.
        Properties* p = _properties[_animations[i]._url];
        if (!p)
        {
            GP_ERROR("The referenced animation data at url '%s' failed to load.",
                     _animations[i]._url.c_str());
            continue;
        }

        node->createAnimation(_animations[i]._animationID, p);
    }
}

//----------------------------------------------------------------------------
PhysicsConstraint* SceneLoader::loadGenericConstraint(const Properties* constraint,
                                                      PhysicsRigidBody* rbA,
                                                      PhysicsRigidBody* rbB)
{
    assert(rbA);
    assert(constraint);
    assert(Game::getInstance()->getPhysicsController());
    PhysicsGenericConstraint* physicsConstraint = nullptr;

    // Create the constraint from the specified properties.
    Quaternion roA;
    Vector3 toA;
    bool offsetSpecified = constraint->getQuaternionFromAxisAngle("rotationOffsetA", &roA);
    offsetSpecified |= constraint->getVector3("translationOffsetA", &toA);

    if (offsetSpecified)
    {
        if (rbB)
        {
            Quaternion roB;
            Vector3 toB;
            constraint->getQuaternionFromAxisAngle("rotationOffsetB", &roB);
            constraint->getVector3("translationOffsetB", &toB);

            physicsConstraint = Game::getInstance()
                                    ->getPhysicsController()
                                    ->createGenericConstraint(rbA, roA, toB, rbB, roB, toB);
        }
        else
        {
            physicsConstraint =
                Game::getInstance()->getPhysicsController()->createGenericConstraint(rbA, roA, toA);
        }
    }
    else
    {
        physicsConstraint =
            Game::getInstance()->getPhysicsController()->createGenericConstraint(rbA, rbB);
    }
    assert(physicsConstraint);

    // Set the optional parameters that were specified.
    Vector3 v;
    if (constraint->getVector3("angularLowerLimit", &v)) physicsConstraint->setAngularLowerLimit(v);
    if (constraint->getVector3("angularUpperLimit", &v)) physicsConstraint->setAngularUpperLimit(v);
    if (constraint->getVector3("linearLowerLimit", &v)) physicsConstraint->setLinearLowerLimit(v);
    if (constraint->getVector3("linearUpperLimit", &v)) physicsConstraint->setLinearUpperLimit(v);

    return physicsConstraint;
}

//----------------------------------------------------------------------------
PhysicsConstraint* SceneLoader::loadHingeConstraint(const Properties* constraint,
                                                    PhysicsRigidBody* rbA,
                                                    PhysicsRigidBody* rbB)
{
    assert(rbA);
    assert(constraint);
    assert(Game::getInstance()->getPhysicsController());
    PhysicsHingeConstraint* physicsConstraint = nullptr;

    // Create the constraint from the specified properties.
    Quaternion roA;
    Vector3 toA;
    constraint->getQuaternionFromAxisAngle("rotationOffsetA", &roA);
    constraint->getVector3("translationOffsetA", &toA);
    if (rbB)
    {
        Quaternion roB;
        Vector3 toB;
        constraint->getQuaternionFromAxisAngle("rotationOffsetB", &roB);
        constraint->getVector3("translationOffsetB", &toB);

        physicsConstraint = Game::getInstance()->getPhysicsController()->createHingeConstraint(rbA,
                                                                                               roA,
                                                                                               toB,
                                                                                               rbB,
                                                                                               roB,
                                                                                               toB);
    }
    else
    {
        physicsConstraint =
            Game::getInstance()->getPhysicsController()->createHingeConstraint(rbA, roA, toA);
    }

    // Load the hinge angle limits (lower and upper) and the hinge bounciness (if specified).
    const auto& limitsString = constraint->getString("limits");
    if (!limitsString.empty())
    {
        float lowerLimit, upperLimit;
        int scanned;
        scanned = sscanf(limitsString.c_str(), "%f,%f", &lowerLimit, &upperLimit);
        if (scanned == 2)
        {
            physicsConstraint->setLimits(MATH_DEG_TO_RAD(lowerLimit), MATH_DEG_TO_RAD(upperLimit));
        }
        else
        {
            float bounciness;
            scanned = sscanf(limitsString.c_str(), "%f,%f,%f", &lowerLimit, &upperLimit, &bounciness);
            if (scanned == 3)
            {
                physicsConstraint->setLimits(MATH_DEG_TO_RAD(lowerLimit),
                                             MATH_DEG_TO_RAD(upperLimit),
                                             bounciness);
            }
            else
            {
                GP_ERROR("Failed to parse 'limits' attribute for hinge constraint '%s'.",
                         constraint->getId());
            }
        }
    }

    return physicsConstraint;
}

//----------------------------------------------------------------------------
Scene* SceneLoader::loadMainSceneData(const Properties* sceneProperties)
{
    assert(sceneProperties);

    // Load the main scene from the specified path.
    Bundle* bundle = Bundle::create(_gpbPath);
    if (!bundle)
    {
        GP_WARN("Failed to load scene GPB file '%s'.", _gpbPath.c_str());
        return nullptr;
    }

    // TODO: Support loading a specific scene from a GPB file using the URL syntax (i.e.
    // "res/scene.gpb#myscene").
    Scene* scene = bundle->loadScene();
    if (!scene)
    {
        GP_WARN("Failed to load scene from '%s'.", _gpbPath.c_str());
        SAFE_RELEASE(bundle);
        return nullptr;
    }

    SAFE_RELEASE(bundle);
    return scene;
}

//----------------------------------------------------------------------------
void SceneLoader::loadPhysics(Properties* physics)
{
    assert(physics);
    assert(Game::getInstance()->getPhysicsController());

    // Go through the supported global physics properties and apply them.
    Vector3 gravity;
    if (physics->getVector3("gravity", &gravity))
        Game::getInstance()->getPhysicsController()->setGravity(gravity);

    Properties* constraint;
    std::string name;
    while ((constraint = physics->getNextNamespace()) != nullptr)
    {
        if (constraint->getNamespace() == "constraint")
        {
            // Get the constraint type.
            std::string type = constraint->getString("type");

            // Attempt to load the first rigid body. If the first rigid body cannot
            // be loaded or found, then continue to the next constraint (error).
            name = constraint->getString("rigidBodyA");
            if (name.empty())
            {
                GP_ERROR("Missing property 'rigidBodyA' for constraint '%s'.", constraint->getId());
                continue;
            }
            Node* rbANode = _scene->findNode(name);
            if (!rbANode)
            {
                GP_ERROR("Node '%s' to be used as 'rigidBodyA' for constraint '%s' cannot be "
                         "found.",
                         name,
                         constraint->getId());
                continue;
            }
            if (!rbANode->getCollisionObject()
                || rbANode->getCollisionObject()->getType() != PhysicsCollisionObject::RIGID_BODY)
            {
                GP_ERROR("Node '%s' to be used as 'rigidBodyA' does not have a rigid body.", name);
                continue;
            }
            PhysicsRigidBody* rbA = static_cast<PhysicsRigidBody*>(rbANode->getCollisionObject());

            // Attempt to load the second rigid body. If the second rigid body is not
            // specified, that is usually okay (only spring constraints require both and
            // we check that below), but if the second rigid body is specified and it doesn't
            // load properly, then continue to the next constraint (error).
            name = constraint->getString("rigidBodyB");
            PhysicsRigidBody* rbB = nullptr;
            if (!name.empty())
            {
                Node* rbBNode = _scene->findNode(name);
                if (!rbBNode)
                {
                    GP_ERROR("Node '%s' to be used as 'rigidBodyB' for constraint '%s' cannot be "
                             "found.",
                             name,
                             constraint->getId());
                    continue;
                }
                if (!rbBNode->getCollisionObject()
                    || rbBNode->getCollisionObject()->getType() != PhysicsCollisionObject::RIGID_BODY)
                {
                    GP_ERROR("Node '%s' to be used as 'rigidBodyB' does not have a rigid body.",
                             name);
                    continue;
                }
                rbB = static_cast<PhysicsRigidBody*>(rbBNode->getCollisionObject());
            }

            PhysicsConstraint* physicsConstraint = nullptr;

            // Load the constraint based on its type.
            if (type == "FIXED")
            {
                physicsConstraint =
                    Game::getInstance()->getPhysicsController()->createFixedConstraint(rbA, rbB);
            }
            else if (type == "GENERIC")
            {
                physicsConstraint = loadGenericConstraint(constraint, rbA, rbB);
            }
            else if (type == "HINGE")
            {
                physicsConstraint = loadHingeConstraint(constraint, rbA, rbB);
            }
            else if (type == "SOCKET")
            {
                physicsConstraint = loadSocketConstraint(constraint, rbA, rbB);
            }
            else if (type == "SPRING")
            {
                physicsConstraint = loadSpringConstraint(constraint, rbA, rbB);
            }
            else
            {
                GP_ERROR("Unsupported physics constraint type '%s'.", type.c_str());
            }

            // If the constraint failed to load, continue on to the next one.
            if (!physicsConstraint)
            {
                GP_ERROR("Failed to create physics constraint.");
                continue;
            }

            // If the breaking impulse was specified, apply it to the constraint.
            if (constraint->exists("breakingImpulse"))
                physicsConstraint->setBreakingImpulse(constraint->getFloat("breakingImpulse"));
        }
        else
        {
            GP_ERROR("Unsupported 'physics' child namespace '%s'.", physics->getNamespace());
        }
    }
}

//----------------------------------------------------------------------------
void SceneLoader::loadReferencedFiles()
{
    // Load all referenced properties files.
    // Pair is type of std::map<std::string, Properties*>
    for (auto& pair : _properties)
    {
        if (pair.second == nullptr)
        {
            std::string fileString;
            std::vector<std::string> namespacePath;
            calculateNamespacePath(pair.first, fileString, namespacePath);

            // Check if the referenced properties file has already been loaded.
            Properties* properties = nullptr;
            std::map<std::string, Properties*>::iterator pffIter =
                _propertiesFromFile.find(fileString);
            if (pffIter != _propertiesFromFile.end() && pffIter->second)
            {
                properties = pffIter->second;
            }
            else
            {
                properties = Properties::create(fileString);
                if (properties == nullptr)
                {
                    GP_WARN("Failed to load referenced properties file '%s'.", fileString.c_str());
                    continue;
                }

                // Add the properties object to the cache.
                _propertiesFromFile.insert(std::make_pair(fileString, properties));
            }

            Properties* p = getPropertiesFromNamespacePath(properties, namespacePath);
            if (!p)
            {
                GP_WARN("Failed to load referenced properties from url '%s'.", pair.first.c_str());
                continue;
            }
            pair.second = p;
        }
    }
}

//----------------------------------------------------------------------------
PhysicsConstraint* SceneLoader::loadSocketConstraint(const Properties* constraint,
                                                     PhysicsRigidBody* rbA,
                                                     PhysicsRigidBody* rbB)
{
    assert(rbA);
    assert(constraint);
    assert(Game::getInstance()->getPhysicsController());

    PhysicsSocketConstraint* physicsConstraint = nullptr;
    Vector3 toA;
    bool offsetSpecified = constraint->getVector3("translationOffsetA", &toA);

    if (offsetSpecified)
    {
        if (rbB)
        {
            Vector3 toB;
            constraint->getVector3("translationOffsetB", &toB);

            physicsConstraint =
                Game::getInstance()->getPhysicsController()->createSocketConstraint(rbA, toA, rbB, toB);
        }
        else
        {
            physicsConstraint =
                Game::getInstance()->getPhysicsController()->createSocketConstraint(rbA, toA);
        }
    }
    else
    {
        physicsConstraint =
            Game::getInstance()->getPhysicsController()->createSocketConstraint(rbA, rbB);
    }

    return physicsConstraint;
}

//----------------------------------------------------------------------------
PhysicsConstraint* SceneLoader::loadSpringConstraint(const Properties* constraint,
                                                     PhysicsRigidBody* rbA,
                                                     PhysicsRigidBody* rbB)
{
    assert(rbA);
    assert(constraint);
    assert(Game::getInstance()->getPhysicsController());

    if (!rbB)
    {
        GP_ERROR("Spring constraints require two rigid bodies.");
        return nullptr;
    }

    PhysicsSpringConstraint* physicsConstraint = nullptr;

    // Create the constraint from the specified properties.
    Quaternion roA, roB;
    Vector3 toA, toB;
    bool offsetsSpecified = constraint->getQuaternionFromAxisAngle("rotationOffsetA", &roA);
    offsetsSpecified |= constraint->getVector3("translationOffsetA", &toA);
    offsetsSpecified |= constraint->getQuaternionFromAxisAngle("rotationOffsetB", &roB);
    offsetsSpecified |= constraint->getVector3("translationOffsetB", &toB);

    if (offsetsSpecified)
    {
        physicsConstraint = Game::getInstance()->getPhysicsController()->createSpringConstraint(rbA,
                                                                                                roA,
                                                                                                toB,
                                                                                                rbB,
                                                                                                roB,
                                                                                                toB);
    }
    else
    {
        physicsConstraint =
            Game::getInstance()->getPhysicsController()->createSpringConstraint(rbA, rbB);
    }
    assert(physicsConstraint);

    // Set the optional parameters that were specified.
    Vector3 v;
    if (constraint->getVector3("angularLowerLimit", &v)) physicsConstraint->setAngularLowerLimit(v);
    if (constraint->getVector3("angularUpperLimit", &v)) physicsConstraint->setAngularUpperLimit(v);
    if (constraint->getVector3("linearLowerLimit", &v)) physicsConstraint->setLinearLowerLimit(v);
    if (constraint->getVector3("linearUpperLimit", &v)) physicsConstraint->setLinearUpperLimit(v);
    if (!constraint->getString("angularDampingX").empty())
        physicsConstraint->setAngularDampingX(constraint->getFloat("angularDampingX"));
    if (!constraint->getString("angularDampingY").empty())
        physicsConstraint->setAngularDampingY(constraint->getFloat("angularDampingY"));
    if (!constraint->getString("angularDampingZ").empty())
        physicsConstraint->setAngularDampingZ(constraint->getFloat("angularDampingZ"));
    if (!constraint->getString("angularStrengthX").empty())
        physicsConstraint->setAngularStrengthX(constraint->getFloat("angularStrengthX"));
    if (!constraint->getString("angularStrengthY").empty())
        physicsConstraint->setAngularStrengthY(constraint->getFloat("angularStrengthY"));
    if (!constraint->getString("angularStrengthZ").empty())
        physicsConstraint->setAngularStrengthZ(constraint->getFloat("angularStrengthZ"));
    if (!constraint->getString("linearDampingX").empty())
        physicsConstraint->setLinearDampingX(constraint->getFloat("linearDampingX"));
    if (!constraint->getString("linearDampingY").empty())
        physicsConstraint->setLinearDampingY(constraint->getFloat("linearDampingY"));
    if (!constraint->getString("linearDampingZ").empty())
        physicsConstraint->setLinearDampingZ(constraint->getFloat("linearDampingZ"));
    if (!constraint->getString("linearStrengthX").empty())
        physicsConstraint->setLinearStrengthX(constraint->getFloat("linearStrengthX"));
    if (!constraint->getString("linearStrengthY").empty())
        physicsConstraint->setLinearStrengthY(constraint->getFloat("linearStrengthY"));
    if (!constraint->getString("linearStrengthZ").empty())
        physicsConstraint->setLinearStrengthZ(constraint->getFloat("linearStrengthZ"));

    return physicsConstraint;
}

//----------------------------------------------------------------------------
void splitURL(const std::string& url, std::string* file, std::string* id)
{
    if (url.empty()) return; // Early exit if the URL is empty.

    id->clear();
    file->clear();

    size_t loc = url.rfind("#");
    if (loc != std::string::npos)
    {
        *file = url.substr(0, loc);
        *id = url.substr(loc + 1);
    }
    else
    {
        *file = url; // No '#' means the entire URL is the file.
    }

    // Check if the file exists
    if (!file->empty() && FileSystem::fileExists(*file))
    {
        // If the file exists, the ID is already set correctly.
        return;
    }

    // If the file does not exist, clear it and set the ID to the original URL
    file->clear();
    *id = url;
}

//----------------------------------------------------------------------------
void SceneLoader::processExactMatchNode(SceneNode& sceneNode, Node* parent, const std::string& id)
{
    Node* node = parent ? parent->findNode(id) : _scene->findNode(id);
    if (node)
    {
        node->setId(sceneNode._nodeID);
        sceneNode._nodes.push_back(node);
    }
    else
    {
        GP_ERROR("Could not find node '%s' in main scene GPB file.", id.c_str());
    }
}

//----------------------------------------------------------------------------
void SceneLoader::processPartialMatchNodes(SceneNode& sceneNode, Node* parent, const std::string& id)
{
    // Search for nodes using a partial match
    std::vector<Node*> nodes;
    size_t nodeCount = parent ? parent->findNodes(id, nodes, true, false)
                              : _scene->findNodes(id, nodes, true, false);
    if (nodeCount > 0)
    {
        for (size_t k = 0; k < nodeCount; ++k)
        {
            // Construct a new node ID using _nodeID plus the remainder of the partial match.
            Node* node = nodes[k];
            std::string newID(sceneNode._nodeID);
            newID += (node->getId() + std::to_string(id.length()));
            node->setId(newID);
            sceneNode._nodes.push_back(node);
        }
    }
    else
    {
        GP_ERROR("Could not find any nodes matching '%s' in main scene GPB file.", id.c_str());
    }
}

//----------------------------------------------------------------------------
void SceneLoader::processExternalFile(SceneNode& sceneNode,
                                      Node* parent,
                                      const std::string& file,
                                      const std::string& id)
{
    // TODO: Revisit this to determine if we should cache Bundle objects for the duration of the
    // scene load to prevent constantly creating/destroying the same externally referenced bundles
    // each time a url with a file is encountered.
    Bundle* tmpBundle = Bundle::create(file);
    if (tmpBundle)
    {
        if (sceneNode._exactMatch)
        {
            Node* node = tmpBundle->loadNode(id, _scene);
            if (node)
            {
                node->setId(sceneNode._nodeID);
                parent ? parent->addChild(node) : _scene->addNode(node);
                sceneNode._nodes.push_back(node);
                SAFE_RELEASE(node);
            }
            else
            {
                GP_ERROR("Could not load node '%s' from GPB file '%s'.", id.c_str(), file.c_str());
            }
        }
        else
        {
            // Search for nodes in the package using a partial match
            size_t objectCount = tmpBundle->getObjectCount();
            size_t matchCount = 0;
            for (size_t k = 0; k < objectCount; ++k)
            {
                std::string objid = tmpBundle->getObjectId(k);
                if (objid.starts_with(id))
                {
                    // This object ID matches (starts with).
                    // Try to load this object as a Node.
                    Node* node = tmpBundle->loadNode(objid);
                    if (node)
                    {
                        // Construct a new node ID using _nodeID plus the remainder of the partial
                        // match.
                        std::string newID(sceneNode._nodeID);
                        newID += (node->getId() + std::to_string(id.length()));
                        node->setId(newID);
                        parent ? parent->addChild(node) : _scene->addNode(node);
                        sceneNode._nodes.push_back(node);
                        SAFE_RELEASE(node);
                        matchCount++;
                    }
                }
            }
            if (matchCount == 0)
            {
                GP_ERROR("Could not find any nodes matching '%s' in GPB file '%s'.",
                         id.c_str(),
                         file.c_str());
            }
        }

        SAFE_RELEASE(tmpBundle);
    }
    else
    {
        GP_ERROR("Failed to load GPB file '%s' for node stitching.", file.c_str());
    }
}

//----------------------------------------------------------------------------
SceneLoader::SceneNodeProperty::SceneNodeProperty(Type type,
                                                  const std::string& value,
                                                  int index,
                                                  bool isUrl)
    : _type(type), _value(value), _isUrl(isUrl), _index(index)
{
}

} // namespace tractor

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

#include "scene/Bundle.h"

#include "animation/Joint.h"
#include "framework/FileSystem.h"
#include "graphics/MeshPart.h"
#include "scene/Scene.h"

// Minimum version numbers supported
constexpr auto BUNDLE_VERSION_MAJOR_REQUIRED = 1;
constexpr auto BUNDLE_VERSION_MINOR_REQUIRED = 2;

constexpr auto BUNDLE_TYPE_SCENE = 1;
constexpr auto BUNDLE_TYPE_NODE = 2;
constexpr auto BUNDLE_TYPE_ANIMATIONS = 3;
constexpr auto BUNDLE_TYPE_ANIMATION = 4;
constexpr auto BUNDLE_TYPE_ANIMATION_CHANNEL = 5;
constexpr auto BUNDLE_TYPE_MODEL = 10;
constexpr auto BUNDLE_TYPE_MATERIAL = 16;
constexpr auto BUNDLE_TYPE_EFFECT = 18;
constexpr auto BUNDLE_TYPE_CAMERA = 32;
constexpr auto BUNDLE_TYPE_LIGHT = 33;
constexpr auto BUNDLE_TYPE_MESH = 34;
constexpr auto BUNDLE_TYPE_MESHPART = 35;
constexpr auto BUNDLE_TYPE_MESHSKIN = 36;
constexpr auto BUNDLE_TYPE_FONT = 128;

// For sanity checking string reads
constexpr auto BUNDLE_MAX_STRING_LENGTH = 5000;

constexpr auto BUNDLE_VERSION_MAJOR_FONT_FORMAT = 1;
constexpr auto BUNDLE_VERSION_MINOR_FONT_FORMAT = 5;

namespace tractor
{

static std::vector<Bundle*> __bundleCache;

//----------------------------------------------------------------------------
Bundle::Bundle(const std::string& path) : _path(path) {}

//----------------------------------------------------------------------------
Bundle::~Bundle()
{
    clearLoadSession();

    // Remove this Bundle from the cache.
    std::vector<Bundle*>::iterator itr = std::find(__bundleCache.begin(), __bundleCache.end(), this);
    if (itr != __bundleCache.end())
    {
        __bundleCache.erase(itr);
    }
}

//----------------------------------------------------------------------------
template <class T> bool Bundle::readArray(unsigned int* length, T** ptr)
{
    assert(length);
    assert(ptr);
    assert(_stream);

    if (!read(length))
    {
        GP_ERROR("Failed to read the length of an array of data (to be read into an array).");
        return false;
    }
    if (*length > 0)
    {
        *ptr = new T[*length];
        if (_stream->read(*ptr, sizeof(T), *length) != *length)
        {
            GP_ERROR("Failed to read an array of data from bundle (into an array).");
            SAFE_DELETE_ARRAY(*ptr);
            return false;
        }
    }
    return true;
}

//----------------------------------------------------------------------------
template <class T> bool Bundle::readArray(unsigned int* length, std::vector<T>* values)
{
    assert(length);
    assert(_stream);

    if (!read(length))
    {
        GP_ERROR("Failed to read the length of an array of data (to be read into a std::vector).");
        return false;
    }
    if (*length > 0 && values)
    {
        values->resize(*length);
        if (_stream->read(&(*values)[0], sizeof(T), *length) != *length)
        {
            GP_ERROR("Failed to read an array of data from bundle (into a std::vector).");
            return false;
        }
    }
    return true;
}

//----------------------------------------------------------------------------
template <class T>
bool Bundle::readArray(unsigned int* length, std::vector<T>* values, unsigned int readSize)
{
    assert(length);
    assert(_stream);
    assert(sizeof(T) >= readSize);

    if (!read(length))
    {
        GP_ERROR("Failed to read the length of an array of data (to be read into a std::vector "
                 "with a specified single element read size).");
        return false;
    }
    if (*length > 0 && values)
    {
        values->resize(*length);
        if (_stream->read(&(*values)[0], readSize, *length) != *length)
        {
            GP_ERROR("Failed to read an array of data from bundle (into a std::vector with a "
                     "specified single element read size).");
            return false;
        }
    }
    return true;
}

//----------------------------------------------------------------------------
static std::string readString(Stream* stream)
{
    assert(stream);

    unsigned int length;
    if (stream->read(&length, 4, 1) != 1)
    {
        GP_ERROR("Failed to read the length of a string from a bundle.");
        return std::string();
    }

    // Sanity check to detect if string length is far too big.
    assert(length < BUNDLE_MAX_STRING_LENGTH);

    std::string str;
    if (length > 0)
    {
        str.resize(length);
        if (stream->read(&str[0], 1, length) != length)
        {
            GP_ERROR("Failed to read string from bundle.");
            return std::string();
        }
    }
    return str;
}

//----------------------------------------------------------------------------
Bundle* Bundle::create(const std::string& path)
{
    // Search the cache for this bundle.
    for (size_t i = 0, count = __bundleCache.size(); i < count; ++i)
    {
        Bundle* p = __bundleCache[i];
        assert(p);
        if (p->_path == path)
        {
            // Found a match
            p->addRef();
            return p;
        }
    }

    // Open the bundle.
    auto stream = FileSystem::open(path);
    if (!stream)
    {
        GP_WARN("Failed to open file '%s'.", path);
        return nullptr;
    }

    // Read the GPB header info.
    char sig[9];
    if (stream->read(sig, 1, 9) != 9 || memcmp(sig, "\xABGPB\xBB\r\n\x1A\n", 9) != 0)
    {
        GP_WARN("Invalid GPB header for bundle '%s'.", path);
        return nullptr;
    }

    // Read version.
    unsigned char version[2];
    if (stream->read(version, 1, 2) != 2)
    {
        GP_WARN("Failed to read GPB version for bundle '%s'.", path);
        return nullptr;
    }
    // Check for the minimal
    if (version[0] != BUNDLE_VERSION_MAJOR_REQUIRED || version[1] < BUNDLE_VERSION_MINOR_REQUIRED)
    {
        GP_WARN("Unsupported version (%d.%d) for bundle '%s' (expected %d.%d).",
                (int)version[0],
                (int)version[1],
                path,
                BUNDLE_VERSION_MAJOR_REQUIRED,
                BUNDLE_VERSION_MINOR_REQUIRED);
        return nullptr;
    }

    // Read ref table.
    unsigned int refCount;
    if (stream->read(&refCount, 4, 1) != 1)
    {
        GP_WARN("Failed to read ref table for bundle '%s'.", path);
        return nullptr;
    }

    // Read all refs.
    Reference* refs = new Reference[refCount];
    for (size_t i = 0; i < refCount; ++i)
    {
        if ((refs[i].id = readString(stream.get())).empty()
            || stream->read(&refs[i].type, 4, 1) != 1 
            || stream->read(&refs[i].offset, 4, 1) != 1)
        {
            GP_WARN("Failed to read ref number %d for bundle '%s'.", i, path);
            SAFE_DELETE_ARRAY(refs);
            return nullptr;
        }
    }

    // Keep file open for faster reading later.
    Bundle* bundle = new Bundle(path);
    bundle->_version[0] = version[0];
    bundle->_version[1] = version[1];
    bundle->_references = std::unique_ptr<Reference[]>(refs);
    bundle->_referencesSpan = std::span<Reference>(bundle->_references.get(), refCount);
    bundle->_stream = std::move(stream);

    return bundle;
}

//----------------------------------------------------------------------------
Bundle::Reference* Bundle::find(const std::string& id) const
{
    assert(_references);

    // Search the ref table for the given id (case-sensitive).
    for (auto& ref : _referencesSpan)
        if (ref.id == id)
            return &ref;

    return nullptr;
}

//----------------------------------------------------------------------------
void Bundle::clearLoadSession()
{
    for (size_t i = 0, count = _meshSkins.size(); i < count; ++i)
    {
        SAFE_DELETE(_meshSkins[i]);
    }
    _meshSkins.clear();
}

//----------------------------------------------------------------------------
const std::string& Bundle::getIdFromOffset() const
{
    assert(_stream);
    return getIdFromOffset((unsigned int)_stream->position());
}

//----------------------------------------------------------------------------
const std::string& Bundle::getIdFromOffset(unsigned int offset) const
{
    // Search the ref table for the given offset.
    if (offset > 0)
    {
        assert(_references);
        for (auto& ref : _referencesSpan)
        {
            if (ref.offset == offset)
            {
                return ref.id;
            }
        }
    }
    return EMPTY_STRING;
}

//----------------------------------------------------------------------------
const std::string& Bundle::getMaterialPath()
{
    if (_materialPath.empty())
    {
        int pos = _path.find_last_of('.');
        if (pos > 2)
        {
            _materialPath = _path.substr(0, pos);
            _materialPath.append(".material");
            if (!FileSystem::fileExists(_materialPath))
            {
                _materialPath.clear();
            }
        }
    }
    return _materialPath;
}

//----------------------------------------------------------------------------
Bundle::Reference* Bundle::seekTo(const std::string& id, unsigned int type)
{
    Reference* ref = find(id);
    if (ref == nullptr)
    {
        GP_ERROR("No object with name '%s' in bundle '%s'.", id, _path.c_str());
        return nullptr;
    }

    if (ref->type != type)
    {
        GP_ERROR("Object '%s' in bundle '%s' has type %d (expected type %d).",
                 id,
                 _path.c_str(),
                 (int)ref->type,
                 (int)type);
        return nullptr;
    }

    // Seek to the offset of this object.
    assert(_stream);
    if (_stream->seek(ref->offset, SEEK_SET) == false)
    {
        GP_ERROR("Failed to seek to object '%s' in bundle '%s'.", id, _path.c_str());
        return nullptr;
    }

    return ref;
}

//----------------------------------------------------------------------------
Bundle::Reference* Bundle::seekToFirstType(unsigned int type)
{
    assert(_references);
    assert(_stream);

    for (auto& ref : _referencesSpan)
    {
        if (ref.type == type)
        {
            if (_stream->seek(ref.offset, SEEK_SET) == false)
            {
                GP_ERROR("Failed to seek to object '%s' in bundle '%s'.",
                         ref.id.c_str(),
                         _path.c_str());
                return nullptr;
            }
            return &ref;
        }
    }

    return nullptr;
}

//----------------------------------------------------------------------------
Scene* Bundle::loadScene(const std::string& id)
{
    clearLoadSession();

    Reference* ref = nullptr;
    if (!id.empty())
    {
        ref = seekTo(id, BUNDLE_TYPE_SCENE);
        if (!ref)
        {
            GP_ERROR("Failed to load scene with id '%s' from bundle.", id);
            return nullptr;
        }
    }
    else
    {
        ref = seekToFirstType(BUNDLE_TYPE_SCENE);
        if (!ref)
        {
            GP_ERROR("Failed to load scene from bundle; bundle contains no scene objects.");
            return nullptr;
        }
    }

    Scene* scene = Scene::create(getIdFromOffset());

    // Read the number of children.
    unsigned int childrenCount;
    if (!read(&childrenCount))
    {
        GP_ERROR("Failed to read the scene's number of children.");
        SAFE_RELEASE(scene);
        return nullptr;
    }
    if (childrenCount > 0)
    {
        // Read each child directly into the scene.
        for (size_t i = 0; i < childrenCount; i++)
        {
            Node* node = readNode(scene, nullptr);
            if (node)
            {
                scene->addNode(node);
                node->release(); // scene now owns node
            }
        }
    }
    // Read active camera.
    std::string xref = readString(_stream.get());
    if (xref.length() > 1 && xref[0] == '#') // TODO: Handle full xrefs
    {
        Node* node = scene->findNode(xref.substr(1), true);
        assert(node);
        Camera* camera = node->getCamera();
        assert(camera);
        scene->setActiveCamera(camera);
    }

    // Read ambient color.
    float red, blue, green;
    if (!read(&red))
    {
        GP_ERROR("Failed to read red component of the scene's ambient color in bundle '%s'.",
                 _path.c_str());
        SAFE_RELEASE(scene);
        return nullptr;
    }
    if (!read(&green))
    {
        GP_ERROR("Failed to read green component of the scene's ambient color in bundle '%s'.",
                 _path.c_str());
        SAFE_RELEASE(scene);
        return nullptr;
    }
    if (!read(&blue))
    {
        GP_ERROR("Failed to read blue component of the scene's ambient color in bundle '%s'.",
                 _path.c_str());
        SAFE_RELEASE(scene);
        return nullptr;
    }
    scene->setAmbientColor(red, green, blue);

    // Parse animations.
    assert(_references);
    assert(_stream);
    for (auto& ref : _referencesSpan)
    {
        if (ref.type == BUNDLE_TYPE_ANIMATIONS)
        {
            // Found a match.
            if (_stream->seek(ref.offset, SEEK_SET) == false)
            {
                GP_ERROR("Failed to seek to object '%s' in bundle '%s'.",
                         ref.id.c_str(),
                         _path.c_str());
                return nullptr;
            }
            readAnimations(scene);
        }
    }

    resolveJointReferences(scene, nullptr);

    return scene;
}

//----------------------------------------------------------------------------
Node* Bundle::loadNode(const std::string& id, Scene* sceneContext)
{
    assert(_references);
    assert(_stream);

    clearLoadSession();

    // Load the node and any referenced joints with node tracking enabled.
    _trackedNodes = new std::map<std::string, Node*>();
    Node* node = loadNode(id, sceneContext, nullptr);
    if (node) resolveJointReferences(sceneContext, node);

    // Load all animations targeting any nodes or mesh skins under this node's hierarchy.
    for (auto& ref : _referencesSpan)
    {
        if (ref.type == BUNDLE_TYPE_ANIMATIONS)
        {
            if (_stream->seek(ref.offset, SEEK_SET) == false)
            {
                GP_ERROR("Failed to seek to object '%s' in bundle '%s'.",
                         ref.id.c_str(),
                         _path.c_str());
                SAFE_DELETE(_trackedNodes);
                return nullptr;
            }

            // Read the number of animations in this object.
            unsigned int animationCount;
            if (!read(&animationCount))
            {
                GP_ERROR("Failed to read the number of animations for object '%s'.", ref.id.c_str());
                SAFE_DELETE(_trackedNodes);
                return nullptr;
            }

            for (size_t j = 0; j < animationCount; j++)
            {
                const std::string id = readString(_stream.get());

                // Read the number of animation channels in this animation.
                unsigned int animationChannelCount;
                if (!read(&animationChannelCount))
                {
                    GP_ERROR("Failed to read the number of animation channels for animation '%s'.",
                             "animationChannelCount",
                             id.c_str());
                    SAFE_DELETE(_trackedNodes);
                    return nullptr;
                }

                Animation* animation = nullptr;
                for (size_t k = 0; k < animationChannelCount; k++)
                {
                    // Read target id.
                    std::string targetId = readString(_stream.get());
                    if (targetId.empty())
                    {
                        GP_ERROR("Failed to read target id for animation '%s'.", id.c_str());
                        SAFE_DELETE(_trackedNodes);
                        return nullptr;
                    }

                    // If the target is one of the loaded nodes/joints, then load the animation.
                    std::map<std::string, Node*>::iterator iter = _trackedNodes->find(targetId);
                    if (iter != _trackedNodes->end())
                    {
                        // Read target attribute.
                        unsigned int targetAttribute;
                        if (!read(&targetAttribute))
                        {
                            GP_ERROR("Failed to read target attribute for animation '%s'.",
                                     id.c_str());
                            SAFE_DELETE(_trackedNodes);
                            return nullptr;
                        }

                        AnimationTarget* target = iter->second;
                        if (!target)
                        {
                            GP_ERROR("Failed to read %s for %s: %s",
                                     "animation target",
                                     targetId.c_str(),
                                     id.c_str());
                            SAFE_DELETE(_trackedNodes);
                            return nullptr;
                        }

                        animation = readAnimationChannelData(animation, id, target, targetAttribute);
                    }
                    else
                    {
                        // Skip over the target attribute.
                        unsigned int data;
                        if (!read(&data))
                        {
                            GP_ERROR("Failed to skip over target attribute for animation '%s'.",
                                     id.c_str());
                            SAFE_DELETE(_trackedNodes);
                            return nullptr;
                        }

                        // Skip the animation channel (passing a target attribute of
                        // 0 causes the animation to not be created).
                        readAnimationChannelData(nullptr, id, nullptr, 0);
                    }
                }
            }
        }
    }

    SAFE_DELETE(_trackedNodes);
    return node;
}

//----------------------------------------------------------------------------
Node* Bundle::loadNode(const std::string& id, Scene* sceneContext, Node* nodeContext)
{
    Node* node = nullptr;

    // Search the passed in loading contexts (scene/node) first to see
    // if we've already loaded this node during this load session.
    if (sceneContext)
    {
        node = sceneContext->findNode(id, true);
        if (node) node->addRef();
    }
    if (node == nullptr && nodeContext)
    {
        node = nodeContext->findNode(id, true);
        if (node) node->addRef();
    }

    if (node == nullptr)
    {
        // If not yet found, search the ref table and read.
        Reference* ref = seekTo(id, BUNDLE_TYPE_NODE);
        if (ref == nullptr)
        {
            return nullptr;
        }

        node = readNode(sceneContext, nodeContext);
    }

    return node;
}

//----------------------------------------------------------------------------
bool Bundle::skipNode()
{
    const auto& id = getIdFromOffset();
    assert(_stream);

    // Skip the node's type.
    unsigned int nodeType;
    if (!read(&nodeType))
    {
        GP_ERROR("Failed to skip node type for node '%s'.", id);
        return false;
    }

    // Skip over the node's transform and parent ID.
    if (_stream->seek(sizeof(float) * 16, SEEK_CUR) == false)
    {
        GP_ERROR("Failed to skip over node transform for node '%s'.", id);
        return false;
    }
    readString(_stream.get());

    // Skip over the node's children.
    unsigned int childrenCount;
    if (!read(&childrenCount))
    {
        GP_ERROR("Failed to skip over node's children count for node '%s'.", id);
        return false;
    }
    else if (childrenCount > 0)
    {
        for (size_t i = 0; i < childrenCount; i++)
        {
            if (!skipNode()) return false;
        }
    }

    // Skip over the node's camera, light, and model attachments.
    Camera* camera = readCamera();
    SAFE_RELEASE(camera);
    Light* light = readLight();
    SAFE_RELEASE(light);
    Model* model = readModel(id);
    SAFE_RELEASE(model);

    return true;
}

//----------------------------------------------------------------------------
Node* Bundle::readNode(Scene* sceneContext, Node* nodeContext)
{
    const auto& id = getIdFromOffset();
    assert(_stream);

    // If we are tracking nodes and it's not in the set yet, add it.
    if (_trackedNodes)
    {
        std::map<std::string, Node*>::iterator iter = _trackedNodes->find(id);
        if (iter != _trackedNodes->end())
        {
            // Skip over this node since we previously read it
            if (!skipNode()) return nullptr;

            iter->second->addRef();
            return iter->second;
        }
    }

    // Read node type.
    unsigned int nodeType;
    if (!read(&nodeType))
    {
        GP_ERROR("Failed to read node type for node '%s'.", id);
        return nullptr;
    }

    Node* node = nullptr;
    switch (nodeType)
    {
        case Node::NODE:
            node = Node::create(id);
            break;
        case Node::JOINT:
            node = Joint::create(id);
            break;
        default:
            return nullptr;
    }

    if (_trackedNodes)
    {
        // Add the new node to the list of tracked nodes
        _trackedNodes->insert(std::make_pair(id, node));
    }

    // If no loading context is set, set this node as the loading context.
    if (sceneContext == nullptr && nodeContext == nullptr)
    {
        nodeContext = node;
    }

    // Read transform.
    float transform[16];
    if (_stream->read(transform, sizeof(float), 16) != 16)
    {
        GP_ERROR("Failed to read transform for node '%s'.", id);
        SAFE_RELEASE(node);
        return nullptr;
    }
    setTransform(*transform, node);

    // Skip the parent ID.
    readString(_stream.get());

    // Read children.
    unsigned int childrenCount;
    if (!read(&childrenCount))
    {
        GP_ERROR("Failed to read children count for node '%s'.", id);
        SAFE_RELEASE(node);
        return nullptr;
    }
    if (childrenCount > 0)
    {
        // Read each child.
        for (size_t i = 0; i < childrenCount; i++)
        {
            // Search the passed in loading contexts (scene/node) first to see
            // if we've already loaded this child node during this load session.
            Node* child = nullptr;
            const auto& id = getIdFromOffset();

            if (sceneContext)
            {
                child = sceneContext->findNode(id, true);
            }
            if (child == nullptr && nodeContext)
            {
                child = nodeContext->findNode(id, true);
            }

            // If the child was already loaded, skip it, otherwise read it
            if (child)
            {
                skipNode();
            }
            else
            {
                child = readNode(sceneContext, nodeContext);
            }

            if (child)
            {
                node->addChild(child);
                child->release(); // 'node' now owns this child
            }
        }
    }

    // Read camera.
    Camera* camera = readCamera();
    if (camera)
    {
        node->setCamera(camera);
        SAFE_RELEASE(camera);
    }

    // Read light.
    Light* light = readLight();
    if (light)
    {
        node->setLight(light);
        SAFE_RELEASE(light);
    }

    // Read model.
    Model* model = readModel(node->getId());
    if (model)
    {
        node->setDrawable(model);
        SAFE_RELEASE(model);
    }
    return node;
}

//----------------------------------------------------------------------------
Camera* Bundle::readCamera()
{
    unsigned char cameraType;
    if (!read(&cameraType))
    {
        GP_ERROR("Failed to load camera type in bundle '%s'.", _path.c_str());
        return nullptr;
    }

    // Check if there isn't a camera to load.
    if (cameraType == 0)
    {
        return nullptr;
    }

    float aspectRatio;
    if (!read(&aspectRatio))
    {
        GP_ERROR("Failed to load camera aspect ratio in bundle '%s'.", _path.c_str());
        return nullptr;
    }

    float nearPlane;
    if (!read(&nearPlane))
    {
        GP_ERROR("Failed to load camera near plane in bundle '%s'.", _path.c_str());
        return nullptr;
    }

    float farPlane;
    if (!read(&farPlane))
    {
        GP_ERROR("Failed to load camera far plane in bundle '%s'.", _path.c_str());
        return nullptr;
    }

    Camera* camera = nullptr;
    if (cameraType == Camera::PERSPECTIVE)
    {
        float fieldOfView;
        if (!read(&fieldOfView))
        {
            GP_ERROR("Failed to load camera field of view in bundle '%s'.", _path.c_str());
            return nullptr;
        }

        camera = Camera::createPerspective(fieldOfView, aspectRatio, nearPlane, farPlane);
    }
    else if (cameraType == Camera::ORTHOGRAPHIC)
    {
        float zoomX;
        if (!read(&zoomX))
        {
            GP_ERROR("Failed to load camera zoomX in bundle '%s'.", _path.c_str());
            return nullptr;
        }

        float zoomY;
        if (!read(&zoomY))
        {
            GP_ERROR("Failed to load camera zoomY in bundle '%s'.", _path.c_str());
            return nullptr;
        }

        camera = Camera::createOrthographic(zoomX, zoomY, aspectRatio, nearPlane, farPlane);
    }
    else
    {
        GP_ERROR("Unsupported camera type (%d) in bundle '%s'.", cameraType, _path.c_str());
        return nullptr;
    }
    return camera;
}

//----------------------------------------------------------------------------
Light* Bundle::readLight()
{
    unsigned char type;
    if (!read(&type))
    {
        GP_ERROR("Failed to load light type in bundle '%s'.", _path.c_str());
        return nullptr;
    }

    // Check if there isn't a light to load.
    if (type == 0)
    {
        return nullptr;
    }

    // Read color.
    float red, blue, green;
    if (!read(&red) || !read(&blue) || !read(&green))
    {
        GP_ERROR("Failed to load light color in bundle '%s'.", _path.c_str());
        return nullptr;
    }
    Vector3 color(red, blue, green);

    Light* light = nullptr;
    if (type == Light::DIRECTIONAL)
    {
        light = Light::createDirectional(color);
    }
    else if (type == Light::POINT)
    {
        float range;
        if (!read(&range))
        {
            GP_ERROR("Failed to load point light range in bundle '%s'.", _path.c_str());
            return nullptr;
        }
        light = Light::createPoint(color, range);
    }
    else if (type == Light::SPOT)
    {
        float range, innerAngle, outerAngle;
        if (!read(&range))
        {
            GP_ERROR("Failed to load spot light range in bundle '%s'.", _path.c_str());
            return nullptr;
        }
        if (!read(&innerAngle))
        {
            GP_ERROR("Failed to load spot light inner angle in bundle '%s'.", _path.c_str());
            return nullptr;
        }
        if (!read(&outerAngle))
        {
            GP_ERROR("Failed to load spot light outer angle in bundle '%s'.", _path.c_str());
            return nullptr;
        }
        light = Light::createSpot(color, range, innerAngle, outerAngle);
    }
    else
    {
        GP_ERROR("Unsupported light type (%d) in bundle '%s'.", type, _path.c_str());
        return nullptr;
    }
    return light;
}

//----------------------------------------------------------------------------
Model* Bundle::readModel(const std::string& nodeId)
{
    std::string xref = readString(_stream.get());
    if (xref.length() > 1 && xref[0] == '#') // TODO: Handle full xrefs
    {
        auto mesh = loadMesh(xref.substr(1), nodeId);
        if (mesh.get())
        {
            Model* model = Model::create(mesh);
            // SAFE_RELEASE(mesh);

            // Read skin.
            unsigned char hasSkin;
            if (!read(&hasSkin))
            {
                GP_ERROR("Failed to load whether model with mesh '%s' has a mesh skin in bundle "
                         "'%s'.",
                         xref.c_str() + 1,
                         _path.c_str());
                return nullptr;
            }
            if (hasSkin)
            {
                MeshSkin* skin = readMeshSkin();
                if (skin)
                {
                    model->setSkin(skin);
                }
            }
            // Read material.
            unsigned int materialCount;
            if (!read(&materialCount))
            {
                GP_ERROR("Failed to load material count for model with mesh '%s' in bundle '%s'.",
                         xref.c_str() + 1,
                         _path.c_str());
                return nullptr;
            }
            if (materialCount > 0)
            {
                for (size_t i = 0; i < materialCount; ++i)
                {
                    std::string materialName = readString(_stream.get());
                    std::string materialPath = getMaterialPath();
                    if (materialPath.length() > 0)
                    {
                        materialPath.append("#");
                        materialPath.append(materialName);
                        Material* material = Material::create(materialPath);
                        if (material)
                        {
                            int partIndex = model->getMesh()->getPartCount() > 0 ? i : -1;
                            model->setMaterial(material, partIndex);
                            SAFE_RELEASE(material);
                        }
                    }
                }
            }
            return model;
        }
    }

    return nullptr;
}

//----------------------------------------------------------------------------
MeshSkin* Bundle::readMeshSkin()
{
    MeshSkin* meshSkin = new MeshSkin();

    // Read bindShape.
    float bindShape[16];
    if (!readMatrix(bindShape))
    {
        GP_ERROR("Failed to load bind shape for mesh skin in bundle '%s'.", _path.c_str());
        SAFE_DELETE(meshSkin);
        return nullptr;
    }
    meshSkin->setBindShape(bindShape);

    // Store the MeshSkinData so we can go back and resolve all joint references later.
    auto& skinData = _meshSkins.emplace_back(new MeshSkinData());
    skinData->skin = meshSkin;

    // Read joint count.
    unsigned int jointCount;
    if (!read(&jointCount))
    {
        GP_ERROR("Failed to load joint count for mesh skin in bundle '%s'.", _path.c_str());
        SAFE_DELETE(meshSkin);
        SAFE_DELETE(skinData);
        return nullptr;
    }
    if (jointCount == 0)
    {
        GP_ERROR("Invalid joint count (must be greater than 0) for mesh skin in bundle '%s'.",
                 _path.c_str());
        SAFE_DELETE(meshSkin);
        SAFE_DELETE(skinData);
        return nullptr;
    }
    meshSkin->setJointCount(jointCount);

    // Read joint xref strings for all joints in the list.
    for (size_t i = 0; i < jointCount; i++)
    {
        skinData->joints.push_back(readString(_stream.get()));
    }

    // Read bind poses.
    unsigned int jointsBindPosesCount;
    if (!read(&jointsBindPosesCount))
    {
        GP_ERROR("Failed to load number of joint bind poses in bundle '%s'.", _path.c_str());
        SAFE_DELETE(meshSkin);
        SAFE_DELETE(skinData);
        return nullptr;
    }
    if (jointsBindPosesCount > 0)
    {
        assert(jointCount * 16 == jointsBindPosesCount);
        float m[16];
        for (size_t i = 0; i < jointCount; i++)
        {
            if (!readMatrix(m))
            {
                GP_ERROR("Failed to load joint bind pose matrix (for joint with index %d) in "
                         "bundle '%s'.",
                         i,
                         _path.c_str());
                SAFE_DELETE(meshSkin);
                SAFE_DELETE(skinData);
                return nullptr;
            }
            skinData->inverseBindPoseMatrices.push_back(m);
        }
    }

    return meshSkin;
}

//----------------------------------------------------------------------------
void Bundle::resolveJointReferences(Scene* sceneContext, Node* nodeContext)
{
    assert(_stream);

    for (size_t i = 0, skinCount = _meshSkins.size(); i < skinCount; ++i)
    {
        MeshSkinData* skinData = _meshSkins[i];
        assert(skinData);
        assert(skinData->skin);

        // Resolve all joints in skin joint list.
        size_t jointCount = skinData->joints.size();
        for (size_t j = 0; j < jointCount; ++j)
        {
            // TODO: Handle full xrefs (not just local # xrefs).
            std::string jointId = skinData->joints[j];
            if (jointId.length() > 1 && jointId[0] == '#')
            {
                jointId = jointId.substr(1);

                Node* n = loadNode(jointId, sceneContext, nodeContext);
                if (n && n->getType() == Node::JOINT)
                {
                    Joint* joint = static_cast<Joint*>(n);
                    joint->setInverseBindPose(skinData->inverseBindPoseMatrices[j]);
                    skinData->skin->setJoint(joint, (unsigned int)j);
                    SAFE_RELEASE(joint);
                }
            }
        }

        // Set the root joint.
        if (jointCount > 0)
        {
            Joint* rootJoint = skinData->skin->getJoint((unsigned int)0);
            Node* node = rootJoint;
            assert(node);
            Node* parent = node->getParent();

            std::vector<Node*> loadedNodes;
            while (true)
            {
                if (parent)
                {
                    if (skinData->skin->getJointIndex(static_cast<Joint*>(parent)) != -1)
                    {
                        // Parent is a joint in the MeshSkin, so treat it as the new root.
                        rootJoint = static_cast<Joint*>(parent);
                    }

                    node = parent;
                    parent = node->getParent();
                }
                else
                {
                    // No parent currently set for this joint.
                    // Lookup its parentID in case it references a node that was not yet loaded as
                    // part of the mesh skin's joint list.
                    std::string nodeId = node->getId();

                    while (true)
                    {
                        // Get the node's type.
                        Reference* ref = find(nodeId);
                        if (ref == nullptr)
                        {
                            GP_ERROR("No object with name '%s' in bundle '%s'.",
                                     nodeId.c_str(),
                                     _path.c_str());
                            return;
                        }

                        // Seek to the current node in the file so we can get it's parent ID.
                        seekTo(nodeId, ref->type);

                        // Skip over the node type (1 unsigned int) and transform (16 floats) and read the parent id.
                        if (_stream->seek(sizeof(unsigned int) + sizeof(float) * 16, SEEK_CUR)
                            == false)
                        {
                            GP_ERROR("Failed to skip over node type and transform for node '%s' in "
                                     "bundle '%s'.",
                                     nodeId.c_str(),
                                     _path.c_str());
                            return;
                        }
                        std::string parentID = readString(_stream.get());

                        if (!parentID.empty())
                            nodeId = parentID;
                        else
                            break;
                    }

                    if (nodeId != rootJoint->getId())
                        loadedNodes.push_back(loadNode(nodeId, sceneContext, nodeContext));

                    break;
                }
            }

            skinData->skin->setRootJoint(rootJoint);

            // Release all the nodes that we loaded since the nodes are now owned by the mesh skin/joints.
            for (size_t i = 0; i < loadedNodes.size(); i++)
            {
                SAFE_RELEASE(loadedNodes[i]);
            }
        }

        // Remove the joint hierarchy from the scene since it is owned by the mesh skin.
        if (sceneContext) sceneContext->removeNode(skinData->skin->_rootNode);

        // Done with this MeshSkinData entry.
        SAFE_DELETE(_meshSkins[i]);
    }
    _meshSkins.clear();
}

//----------------------------------------------------------------------------
void Bundle::readAnimation(Scene* scene)
{
    const std::string animationId = readString(_stream.get());

    // Read the number of animation channels in this animation.
    unsigned int animationChannelCount;
    if (!read(&animationChannelCount))
    {
        GP_ERROR("Failed to read animation channel count for animation '%s'.", animationId.c_str());
        return;
    }

    Animation* animation = nullptr;
    for (size_t i = 0; i < animationChannelCount; i++)
    {
        animation = readAnimationChannel(scene, animation, animationId);
    }
}

//----------------------------------------------------------------------------
void Bundle::readAnimations(Scene* scene)
{
    // Read the number of animations in this object.
    unsigned int animationCount;
    if (!read(&animationCount))
    {
        GP_ERROR("Failed to read the number of animations in the scene.");
        return;
    }

    for (size_t i = 0; i < animationCount; i++)
    {
        readAnimation(scene);
    }
}

//----------------------------------------------------------------------------
Animation* Bundle::readAnimationChannel(Scene* scene,
                                        Animation* animation,
                                        const std::string& animationId)
{
    // Read target id.
    std::string targetId = readString(_stream.get());
    if (targetId.empty())
    {
        GP_ERROR("Failed to read target id for animation '%s'.", animationId);
        return nullptr;
    }

    // Read target attribute.
    unsigned int targetAttribute;
    if (!read(&targetAttribute))
    {
        GP_ERROR("Failed to read target attribute for animation '%s'.", animationId);
        return nullptr;
    }

    AnimationTarget* target = nullptr;

    // Search for a node that matches the target.
    if (!target)
    {
        target = scene->findNode(targetId);
        if (!target)
        {
            GP_ERROR("Failed to find the animation target (with id '%s') for animation '%s'.",
                     targetId.c_str(),
                     animationId);
            return nullptr;
        }
    }

    return readAnimationChannelData(animation, animationId, target, targetAttribute);
}

//----------------------------------------------------------------------------
Animation* Bundle::readAnimationChannelData(Animation* animation,
                                            const std::string& id,
                                            AnimationTarget* target,
                                            unsigned int targetAttribute)
{
    std::vector<unsigned int> keyTimes;
    std::vector<float> values;
    std::vector<float> tangentsIn;
    std::vector<float> tangentsOut;
    std::vector<unsigned int> interpolation;

    // Length of the arrays.
    unsigned int keyTimesCount;
    unsigned int valuesCount;
    unsigned int tangentsInCount;
    unsigned int tangentsOutCount;
    unsigned int interpolationCount;

    // Read key times.
    if (!readArray(&keyTimesCount, &keyTimes, sizeof(unsigned int)))
    {
        GP_ERROR("Failed to read key times for animation '%s'.", id);
        return nullptr;
    }

    // Read key values.
    if (!readArray(&valuesCount, &values))
    {
        GP_ERROR("Failed to read key values for animation '%s'.", id);
        return nullptr;
    }

    // Read in-tangents.
    if (!readArray(&tangentsInCount, &tangentsIn))
    {
        GP_ERROR("Failed to read in tangents for animation '%s'.", id);
        return nullptr;
    }

    // Read out-tangents.
    if (!readArray(&tangentsOutCount, &tangentsOut))
    {
        GP_ERROR("Failed to read out tangents for animation '%s'.", id);
        return nullptr;
    }

    // Read interpolations.
    if (!readArray(&interpolationCount, &interpolation, sizeof(unsigned int)))
    {
        GP_ERROR("Failed to read the interpolation values for animation '%s'.", id);
        return nullptr;
    }

    if (targetAttribute > 0)
    {
        assert(target);
        assert(keyTimes.size() > 0 && values.size() > 0);
        if (animation == nullptr)
        {
            // TODO: This code currently assumes LINEAR only.
            animation = target->createAnimation(id,
                                                targetAttribute,
                                                keyTimesCount,
                                                &keyTimes[0],
                                                &values[0],
                                                Curve::LINEAR);
        }
        else
        {
            animation->createChannel(target,
                                     targetAttribute,
                                     keyTimesCount,
                                     &keyTimes[0],
                                     &values[0],
                                     Curve::LINEAR);
        }
    }

    return animation;
}

std::shared_ptr<Mesh> Bundle::loadMesh(const std::string& id) { return loadMesh(id, nullptr); }

//----------------------------------------------------------------------------
std::shared_ptr<Mesh> Bundle::loadMesh(const std::string& id, const std::string& nodeId)
{
    assert(_stream);

    // Save the file position.
    long position = _stream->position();
    if (position == -1L)
    {
        GP_ERROR("Failed to save the current file position before loading mesh '%s'.", id);
        return nullptr;
    }

    // Seek to the specified mesh.
    Reference* ref = seekTo(id, BUNDLE_TYPE_MESH);
    if (ref == nullptr)
    {
        GP_ERROR("Failed to locate ref for mesh '%s'.", id);
        return nullptr;
    }

    // Read mesh data.
    auto meshData = readMeshData();
    if (meshData == nullptr)
    {
        GP_ERROR("Failed to load mesh data for mesh '%s'.", id);
        return nullptr;
    }

    // Create mesh.
    auto mesh = Mesh::createMesh(meshData->vertexFormat, meshData->vertexCount, false);
    if (mesh == nullptr)
    {
        GP_ERROR("Failed to create mesh '%s'.", id);
        return nullptr;
    }

    mesh->_url = _path;
    mesh->_url += "#";
    mesh->_url += id;

    mesh->setVertexData((float*)meshData->vertexData, 0, meshData->vertexCount);

    mesh->_boundingBox.set(meshData->boundingBox);
    mesh->_boundingSphere.set(meshData->boundingSphere);

    // Create mesh parts.
    for (auto& partData : meshData->parts)
    {
        assert(partData);

        auto part =
            mesh->addPart(partData->primitiveType, partData->indexFormat, partData->indexCount, false);
        if (part == nullptr)
        {
            GP_ERROR("Failed to create mesh part for mesh '%s'.", id);
            return nullptr;
        }
        part->setIndexData(partData->indexData, 0, partData->indexCount);
    }

    // Restore file pointer.
    if (_stream->seek(position, SEEK_SET) == false)
    {
        GP_ERROR("Failed to restore file pointer after loading mesh '%s'.", id);
        return nullptr;
    }

    return mesh;
}

//----------------------------------------------------------------------------
std::unique_ptr<Bundle::MeshData> Bundle::readMeshData()
{
    // Read vertex format/elements.
    unsigned int vertexElementCount;
    if (_stream->read(&vertexElementCount, 4, 1) != 1)
    {
        GP_ERROR("Failed to load vertex element count.");
        return nullptr;
    }
    if (vertexElementCount < 1)
    {
        GP_ERROR(
            "Failed to load mesh data; invalid vertex element count (must be greater than 0).");
        return nullptr;
    }

    VertexFormat::Element* vertexElements = new VertexFormat::Element[vertexElementCount];
    for (size_t i = 0; i < vertexElementCount; ++i)
    {
        unsigned int vUsage, vSize;
        if (_stream->read(&vUsage, 4, 1) != 1)
        {
            GP_ERROR("Failed to load vertex usage.");
            SAFE_DELETE_ARRAY(vertexElements);
            return nullptr;
        }
        if (_stream->read(&vSize, 4, 1) != 1)
        {
            GP_ERROR("Failed to load vertex size.");
            SAFE_DELETE_ARRAY(vertexElements);
            return nullptr;
        }

        vertexElements[i].usage = (VertexFormat::Usage)vUsage;
        vertexElements[i].size = vSize;
    }

    auto meshData = std::make_unique<MeshData>(VertexFormat(vertexElements, vertexElementCount));
    SAFE_DELETE_ARRAY(vertexElements);

    // Read vertex data.
    unsigned int vertexByteCount;
    if (_stream->read(&vertexByteCount, 4, 1) != 1)
    {
        GP_ERROR("Failed to load vertex byte count.");
        // SAFE_DELETE(meshData);
        meshData.release();
        return nullptr;
    }
    if (vertexByteCount == 0)
    {
        GP_ERROR("Failed to load mesh data; invalid vertex byte count of 0.");
        meshData.release();
        // SAFE_DELETE(meshData);
        return nullptr;
    }

    assert(meshData->vertexFormat.getVertexSize());
    meshData->vertexCount = vertexByteCount / meshData->vertexFormat.getVertexSize();
    meshData->vertexData = new unsigned char[vertexByteCount];
    if (_stream->read(meshData->vertexData, 1, vertexByteCount) != vertexByteCount)
    {
        GP_ERROR("Failed to load vertex data.");
        meshData.release();
        // SAFE_DELETE(meshData);
        return nullptr;
    }

    // Read mesh bounds (bounding box and bounding sphere).
    if (_stream->read(&meshData->boundingBox.min.x, 4, 3) != 3
        || _stream->read(&meshData->boundingBox.max.x, 4, 3) != 3)
    {
        GP_ERROR("Failed to load mesh bounding box.");
        meshData.release();
        // SAFE_DELETE(meshData);
        return nullptr;
    }
    if (_stream->read(&meshData->boundingSphere.center.x, 4, 3) != 3
        || _stream->read(&meshData->boundingSphere.radius, 4, 1) != 1)
    {
        GP_ERROR("Failed to load mesh bounding sphere.");
        meshData.release();
        // SAFE_DELETE(meshData);
        return nullptr;
    }

    // Read mesh parts.
    unsigned int meshPartCount;
    if (_stream->read(&meshPartCount, 4, 1) != 1)
    {
        GP_ERROR("Failed to load mesh part count.");
        meshData.release();
        // SAFE_DELETE(meshData);
        return nullptr;
    }
    for (size_t i = 0; i < meshPartCount; ++i)
    {
        // Read primitive type, index format and index count.
        unsigned int pType, iFormat, iByteCount;
        if (_stream->read(&pType, 4, 1) != 1)
        {
            GP_ERROR("Failed to load primitive type for mesh part with index %d.", i);
            meshData.release();
            // SAFE_DELETE(meshData);
            return nullptr;
        }
        if (_stream->read(&iFormat, 4, 1) != 1)
        {
            GP_ERROR("Failed to load index format for mesh part with index %d.", i);
            meshData.release();
            // SAFE_DELETE(meshData);
            return nullptr;
        }
        if (_stream->read(&iByteCount, 4, 1) != 1)
        {
            GP_ERROR("Failed to load index byte count for mesh part with index %d.", i);
            meshData.release();
            // SAFE_DELETE(meshData);
            return nullptr;
        }

        auto& partData = meshData->parts.emplace_back(new MeshPartData());

        partData->primitiveType = (Mesh::PrimitiveType)pType;
        partData->indexFormat = (Mesh::IndexFormat)iFormat;

        unsigned int indexSize = 0;
        switch (partData->indexFormat)
        {
            case Mesh::INDEX8:
                indexSize = 1;
                break;
            case Mesh::INDEX16:
                indexSize = 2;
                break;
            case Mesh::INDEX32:
                indexSize = 4;
                break;
            default:
                GP_ERROR("Unsupported index format for mesh part with index %d.", i);
                return nullptr;
        }

        assert(indexSize);
        partData->indexCount = iByteCount / indexSize;

        partData->indexData = new unsigned char[iByteCount];
        if (_stream->read(partData->indexData, 1, iByteCount) != iByteCount)
        {
            GP_ERROR("Failed to read index data for mesh part with index %d.", i);
            meshData.release();
            // SAFE_DELETE(meshData);
            return nullptr;
        }
    }

    return meshData;
}

//----------------------------------------------------------------------------
std::unique_ptr<Bundle::MeshData> Bundle::readMeshData(const std::string& url)
{
    if (url.empty())
    {
        GP_ERROR("Mesh data URL must be non-empty.");
        return nullptr;
    }

    // Parse URL (formatted as 'bundle#id').
    std::string urlstring(url);
    size_t pos = urlstring.find('#');
    if (pos == std::string::npos)
    {
        GP_ERROR("Invalid mesh data URL '%s' (must be of the form 'bundle#id').", url);
        return nullptr;
    }

    std::string file = urlstring.substr(0, pos);
    std::string id = urlstring.substr(pos + 1);

    // Load bundle.
    Bundle* bundle = Bundle::create(file);
    if (bundle == nullptr)
    {
        GP_ERROR("Failed to load bundle '%s'.", file.c_str());
        return nullptr;
    }

    // Seek to mesh with specified ID in bundle.
    Reference* ref = bundle->seekTo(id, BUNDLE_TYPE_MESH);
    if (ref == nullptr)
    {
        GP_ERROR("Failed to load ref from bundle '%s' for mesh with id '%s'.",
                 file.c_str(),
                 id.c_str());
        return nullptr;
    }

    // Read mesh data from current file position.
    auto meshData = bundle->readMeshData();

    SAFE_RELEASE(bundle);

    return meshData;
}

//----------------------------------------------------------------------------
Font* Bundle::loadFont(const std::string& id)
{
    assert(_stream);

    // Seek to the specified font.
    Reference* ref = seekTo(id, BUNDLE_TYPE_FONT);
    if (ref == nullptr)
    {
        GP_ERROR("Failed to load ref for font '%s'.", id);
        return nullptr;
    }

    // Read font family.
    std::string family = readString(_stream.get());
    if (family.empty())
    {
        GP_ERROR("Failed to read font family for font '%s'.", id);
        return nullptr;
    }

    // Read font style
    unsigned int style;
    if (_stream->read(&style, 4, 1) != 1)
    {
        GP_ERROR("Failed to read style for font '%s'.", id);
        return nullptr;
    }

    // In bundle version 1.4 we introduced storing multiple font sizes per font
    unsigned int fontSizeCount = 1;
    if (getVersionMajor() >= 1 && getVersionMinor() >= 4)
    {
        if (_stream->read(&fontSizeCount, 4, 1) != 1)
        {
            GP_ERROR("Failed to read font size count for font '%s'.", id);
            return nullptr;
        }
    }

    Font* masterFont = nullptr;

    for (size_t i = 0; i < fontSizeCount; ++i)
    {
        // Read font size
        unsigned int size;
        if (_stream->read(&size, 4, 1) != 1)
        {
            GP_ERROR("Failed to read size for font '%s'.", id);
            return nullptr;
        }

        // Read character set.
        std::string charset = readString(_stream.get());

        // Read font glyphs.
        unsigned int glyphCount;
        if (_stream->read(&glyphCount, 4, 1) != 1)
        {
            GP_ERROR("Failed to read glyph count for font '%s'.", id);
            return nullptr;
        }
        if (glyphCount == 0)
        {
            GP_ERROR("Invalid glyph count (must be greater than 0) for font '%s'.", id);
            return nullptr;
        }

        Font::Glyph* glyphs = new Font::Glyph[glyphCount];
        for (unsigned j = 0; j < glyphCount; j++)
        {
            if (_stream->read(&glyphs[j].code, 4, 1) != 1)
            {
                GP_ERROR("Failed to read glyph #%d code for font '%s'.", j, id);
                SAFE_DELETE_ARRAY(glyphs);
                return nullptr;
            }
            if (_stream->read(&glyphs[j].width, 4, 1) != 1)
            {
                GP_ERROR("Failed to read glyph #%d width for font '%s'.", j, id);
                SAFE_DELETE_ARRAY(glyphs);
                return nullptr;
            }
            if (getVersionMajor() >= 1 && getVersionMinor() >= 5)
            {
                if (_stream->read(&glyphs[j].bearingX, 4, 1) != 1)
                {
                    GP_ERROR("Failed to read glyph #%d bearingX for font '%s'.", j, id);
                    SAFE_DELETE_ARRAY(glyphs);
                    return nullptr;
                }
                if (_stream->read(&glyphs[j].advance, 4, 1) != 1)
                {
                    GP_ERROR("Failed to read glyph #%d advance for font '%s'.", j, id);
                    SAFE_DELETE_ARRAY(glyphs);
                    return nullptr;
                }
            }
            else
            {
                // Fallback values for older GBP format.
                glyphs[j].bearingX = 0;
                glyphs[j].advance = glyphs[j].width;
            }
            if (_stream->read(&glyphs[j].uvs, 4, 4) != 4)
            {
                GP_ERROR("Failed to read glyph #%d uvs for font '%s'.", j, id);
                SAFE_DELETE_ARRAY(glyphs);
                return nullptr;
            }
        }

        // Read texture attributes.
        unsigned int width, height, textureByteCount;
        if (_stream->read(&width, 4, 1) != 1)
        {
            GP_ERROR("Failed to read texture width for font '%s'.", id);
            SAFE_DELETE_ARRAY(glyphs);
            return nullptr;
        }
        if (_stream->read(&height, 4, 1) != 1)
        {
            GP_ERROR("Failed to read texture height for font '%s'.", id);
            SAFE_DELETE_ARRAY(glyphs);
            return nullptr;
        }
        if (_stream->read(&textureByteCount, 4, 1) != 1)
        {
            GP_ERROR("Failed to read texture byte count for font '%s'.", id);
            SAFE_DELETE_ARRAY(glyphs);
            return nullptr;
        }
        if (textureByteCount != (width * height))
        {
            GP_ERROR("Invalid texture byte count for font '%s'.", id);
            SAFE_DELETE_ARRAY(glyphs);
            return nullptr;
        }

        // Read texture data.
        unsigned char* textureData = new unsigned char[textureByteCount];
        if (_stream->read(textureData, 1, textureByteCount) != textureByteCount)
        {
            GP_ERROR("Failed to read texture data for font '%s'.", id);
            SAFE_DELETE_ARRAY(glyphs);
            SAFE_DELETE_ARRAY(textureData);
            return nullptr;
        }

        unsigned int format = Font::BITMAP;

        // In bundle version 1.3 we added a format field
        if (getVersionMajor() >= 1 && getVersionMinor() >= 3)
        {
            if (_stream->read(&format, 4, 1) != 1)
            {
                GP_ERROR("Failed to font format'%u'.", format);
                SAFE_DELETE_ARRAY(glyphs);
                SAFE_DELETE_ARRAY(textureData);
                return nullptr;
            }
        }

        // Create the texture for the font.
        Texture* texture = Texture::create(Texture::ALPHA, width, height, textureData, true);

        // Free the texture data (no longer needed).
        SAFE_DELETE_ARRAY(textureData);

        if (texture == nullptr)
        {
            GP_ERROR("Failed to create texture for font '%s'.", id);
            SAFE_DELETE_ARRAY(glyphs);
            return nullptr;
        }

        // Create the font for this size
        Font* font =
            Font::create(family, Font::PLAIN, size, glyphs, glyphCount, texture, (Font::Format)format);

        // Free the glyph array.
        SAFE_DELETE_ARRAY(glyphs);

        // Release the texture since the Font now owns it.
        SAFE_RELEASE(texture);

        if (font)
        {
            font->_path = _path;
            font->_id = id;

            if (masterFont)
                masterFont->_sizes.push_back(font);
            else
                masterFont = font;
        }
    }

    return masterFont;
}

//----------------------------------------------------------------------------
void Bundle::setTransform(const float& values, Transform* transform)
{
    assert(transform);

    // Load array into transform.
    Matrix matrix(&values);
    Vector3 scale, translation;
    Quaternion rotation;
    matrix.decompose(&scale, &rotation, &translation);
    transform->setScale(scale);
    transform->setTranslation(translation);
    transform->setRotation(rotation);
}

//----------------------------------------------------------------------------
const std::string& Bundle::getObjectId(unsigned int index) const
{
    assert(_references);

    if (index >= _referencesSpan.size())
    {
        return EMPTY_STRING;
    }

    return _references[index].id;
}

//----------------------------------------------------------------------------
Bundle::MeshPartData::~MeshPartData() { SAFE_DELETE_ARRAY(indexData); }

//----------------------------------------------------------------------------
Bundle::MeshData::MeshData(const VertexFormat& vertexFormat) : vertexFormat(vertexFormat) {}

//----------------------------------------------------------------------------
Bundle::MeshData::~MeshData()
{
    SAFE_DELETE_ARRAY(vertexData);

    for (size_t i = 0; i < parts.size(); ++i)
    {
        SAFE_DELETE(parts[i]);
    }
}

} // namespace tractor

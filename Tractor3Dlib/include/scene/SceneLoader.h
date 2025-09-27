#pragma once

#include "pch.h"

#include "graphics/Mesh.h"
#include "physics/PhysicsRigidBody.h"
#include "scene/Properties.h"
#include "scene/Scene.h"

namespace tractor
{

/**
 * Defines an internal helper class for loading scenes from .scene files.
 *
 * @script{ignore}
 */
class SceneLoader
{
    friend class Scene;

  private:
    /**
     * Loads a scene using the data from the Properties object defined at the specified URL,
     * where the URL is of the format
     * "<file-path>.<extension>#<namespace-id>/<namespace-id>/.../<namespace-id>" (and
     * "#<namespace-id>/<namespace-id>/.../<namespace-id>" is optional).
     *
     * @param url The URL pointing to the Properties object defining the scene.
     */
    static Scene* load(const std::string& url);

    /**
     * Helper structures and functions for SceneLoader::load(const char*).
     */
    struct SceneAnimation
    {
        SceneAnimation(const std::string& animationID, const std::string& targetID, std::string url)
            : _animationID(animationID), _targetID(targetID), _url(url)
        {
        }

        std::string _animationID;
        std::string _targetID;
        std::string _url;
    };

    struct SceneNodeProperty
    {
        enum Type
        {
            AUDIO = 1,
            MATERIAL = 2,
            PARTICLE = 4,
            TERRAIN = 8,
            LIGHT = 16,
            CAMERA = 32,
            COLLISION_OBJECT = 64,
            TRANSLATE = 128,
            ROTATE = 256,
            SCALE = 512,
            URL = 1024,
            SCRIPT = 2048,
            SPRITE = 4096,
            TILESET = 8192,
            TEXT = 16384,
            ENABLED = 32768
        };

        SceneNodeProperty(Type type, const std::string& value, int index, bool isUrl);

        Type _type;
        std::string _value{};
        bool _isUrl{ false };
        int _index{ 0 };
    };

    struct SceneNode
    {
        SceneNode() = default;

        std::string _nodeID{};
        bool _exactMatch{ true };
        Properties* _namespace{ nullptr };
        std::vector<Node*> _nodes{}; // list of nodes sharing properties defined in this SceneNode
        std::vector<SceneNode> _children{}; // list of unique child nodes
        std::vector<SceneNodeProperty> _properties{};
        std::map<std::string, std::string> _tags{};
    };

    SceneLoader() = default;

    Scene* loadInternal(const std::string& url);

    void applyTags(SceneNode& sceneNode);

    void addSceneAnimation(const std::string& animationID,
                           const std::string& targetID,
                           const std::string& url);

    void addSceneNodeProperty(SceneNode& sceneNode,
                              SceneNodeProperty::Type type,
                              const std::string& value = EMPTY_STRING,
                              bool supportsUrl = false,
                              int index = 0);

    void applyNodeProperties(const Properties* sceneProperties, size_t typeFlags);

    void applyNodeProperties(SceneNode& sceneNode, const Properties* sceneProperties, size_t typeFlags);

    void applyNodeProperty(SceneNode& sceneNode,
                           Node* node,
                           const Properties* sceneProperties,
                           const SceneNodeProperty& snp);
    /**
     * @brief Applies or updates URLs for nodes.
     */
    void applyNodeUrls();

    void applyNodeUrls(SceneNode& sceneNode, Node* parent);

    void buildReferenceTables(Properties* sceneProperties);

    void parseNode(Properties* ns, SceneNode* parent, const std::string& path);

    void calculateNodesWithMeshRigidBodies(const Properties* sceneProperties);

    void createAnimations();

    PhysicsConstraint* loadGenericConstraint(const Properties* constraint,
                                             PhysicsRigidBody* rbA,
                                             PhysicsRigidBody* rbB);

    PhysicsConstraint* loadHingeConstraint(const Properties* constraint,
                                           PhysicsRigidBody* rbA,
                                           PhysicsRigidBody* rbB);

    Scene* loadMainSceneData(const Properties* sceneProperties);

    void loadPhysics(Properties* physics);

    void loadReferencedFiles();

    PhysicsConstraint* loadSocketConstraint(const Properties* constraint,
                                            PhysicsRigidBody* rbA,
                                            PhysicsRigidBody* rbB);

    PhysicsConstraint* loadSpringConstraint(const Properties* constraint,
                                            PhysicsRigidBody* rbA,
                                            PhysicsRigidBody* rbB);

    void processExactMatchNode(SceneNode& sceneNode, Node* parent, const std::string& id);

    void processPartialMatchNodes(SceneNode& sceneNode, Node* parent, const std::string& id);

    void processExternalFile(SceneNode& sceneNode,
                             Node* parent,
                             const std::string& file,
                             const std::string& id);

    std::map<std::string, Properties*> _propertiesFromFile; // Holds the properties object for a given file.
    std::map<std::string, Properties*> _properties; // Holds the properties object for a given URL.
    std::vector<SceneAnimation> _animations; // Holds the animations declared in the .scene file.
    std::vector<SceneNode> _sceneNodes; // Holds all the nodes+properties declared in the .scene file.
    std::string _gpbPath;               // The path of the main GPB for the scene being loaded.
    std::string _path;                  // The path of the scene file being loaded.
    Scene* _scene{ nullptr };           // The scene being loaded
};

/**
 * Utility function for splitting up a URL of the form 'file#id', where one or both of file and id
 * may be empty.
 *
 * @param url The url to split.
 * @param file The out parameter containing the file part of the url.
 * @param id The out parameter containing the id part of the url.
 */
void splitURL(const std::string& url, std::string* file, std::string* id);

} // namespace tractor

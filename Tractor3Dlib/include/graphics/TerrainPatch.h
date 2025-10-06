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

#include "graphics/Model.h"
#include "renderer/Camera.h"

namespace tractor
{

class Terrain;
class TerrainAutoBindingResolver;

/**
 * Defines a single patch for a Terrain.
 */
class TerrainPatch : public Camera::Listener
{
    friend class Terrain;
    friend class TerrainAutoBindingResolver;

  public:
    /**
     * Gets the number of material for this patch for all level of details.
     *
     * @return The number of material for this patch for all level of details.
     */
    size_t getMaterialCount() const noexcept { return _levels.size(); };

    /**
     * Gets the material for the specified level of detail index or -1 for the current level of
     * detail based on the scene camera.
     *
     * @param index The index for the level of detail to get the material for.
     */
    Material* getMaterial(int index = -1) const;

    /**
     * Gets the local bounding box for this patch, at the base LOD level.
     */
    const BoundingBox& getBoundingBox(bool worldSpace) const;

    /**
     * @see Camera::Listener
     */
    void cameraChanged(Camera* camera) { _bits |= TERRAINPATCH_DIRTY_LEVEL; };

    /**
     * Internal use only.
     *
     * @script{ignore}
     */
    static std::string passCallback(Pass* pass, void* cookie);

  private:
    /**
     * Constructor.
     */
    TerrainPatch() = default;

    /**
     * Hidden copy constructor.
     */
    TerrainPatch(const TerrainPatch&);

    /**
     * Hidden copy assignment operator.
     */
    TerrainPatch& operator=(const TerrainPatch&) = delete;

    /**
     * Destructor.
     */
    ~TerrainPatch();

    struct Layer
    {
        Layer() = default;

        Layer(const Layer&) = default;

        ~Layer() = default;

        Layer& operator=(const Layer&) = delete;

        int index{ 0 };
        int row{ -1 };
        int column{ -1 };
        int textureIndex{ -1 };
        int blendIndex{ -1 };
        int blendChannel{ 0 };
        Vector2 textureRepeat;
    };

    struct Level
    {
        Model* model{ nullptr };

        Level() = default;
    };

    struct LayerCompare
    {
        bool operator()(const Layer* lhs, const Layer* rhs) const;
    };

    static TerrainPatch* create(Terrain* terrain,
                                unsigned int index,
                                unsigned int row,
                                unsigned int column,
                                float* heights,
                                unsigned int width,
                                unsigned int height,
                                unsigned int x1,
                                unsigned int z1,
                                unsigned int x2,
                                unsigned int z2,
                                float xOffset,
                                float zOffset,
                                unsigned int maxStep,
                                float verticalSkirtSize);

    void addLOD(float* heights,
                unsigned int width,
                unsigned int height,
                unsigned int x1,
                unsigned int z1,
                unsigned int x2,
                unsigned int z2,
                float xOffset,
                float zOffset,
                unsigned int step,
                float verticalSkirtSize);

    bool setLayer(int index,
                  const std::string& texturePath,
                  const Vector2& textureRepeat,
                  const std::string& blendPath,
                  int blendChannel);

    void deleteLayer(Layer* layer);

    int addSampler(const std::string& path);

    unsigned int draw(bool wireframe);

    bool updateMaterial();

    unsigned int computeLOD(Camera* camera, const BoundingBox& worldBounds);

    const Vector3& getAmbientColor() const;

    void setMaterialDirty() { _bits |= TERRAINPATCH_DIRTY_MATERIAL; };

    float computeHeight(float* heights, unsigned int width, unsigned int x, unsigned int z);

    void updateNodeBindings();

    std::string passCreated(Pass* pass);

  private:
    static constexpr auto TERRAINPATCH_DIRTY_MATERIAL = 1;
    static constexpr auto TERRAINPATCH_DIRTY_BOUNDS = 2;
    static constexpr auto TERRAINPATCH_DIRTY_LEVEL = 4;

    Terrain* _terrain{ nullptr };
    unsigned int _index{ 0 };
    unsigned int _row{ 0 };
    unsigned int _column{ 0 };
    std::vector<Level*> _levels{};
    std::set<Layer*, LayerCompare> _layers{};
    std::vector<Texture::Sampler*> _samplers{};
    mutable BoundingBox _boundingBox{};
    mutable BoundingBox _boundingBoxWorld{};
    mutable Camera* _camera{ nullptr };
    mutable unsigned int _level{ 0 };
    mutable int _bits{ TERRAINPATCH_DIRTY_MATERIAL | TERRAINPATCH_DIRTY_BOUNDS
                       | TERRAINPATCH_DIRTY_LEVEL };
};

} // namespace tractor

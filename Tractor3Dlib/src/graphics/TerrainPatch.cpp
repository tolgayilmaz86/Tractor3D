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

#include "graphics/TerrainPatch.h"

#include "framework/Game.h"
#include "graphics/MeshPart.h"
#include "graphics/Terrain.h"
#include "renderer/Pass.h"
#include "scene/Scene.h"

namespace tractor
{

/**
 * Custom material auto-binding resolver for terrain.
 * @script{ignore}
 */
class TerrainAutoBindingResolver : RenderState::AutoBindingResolver
{
    bool resolveAutoBinding(const std::string& autoBinding, Node* node, MaterialParameter* parameter);
};
static TerrainAutoBindingResolver __autoBindingResolver;
static int __currentPatchIndex = -1;

//----------------------------------------------------------------------------
TerrainPatch::~TerrainPatch()
{
    while (_layers.size() > 0)
    {
        deleteLayer(*_layers.begin());
    }

    if (auto camera = _camera.lock())
    {
        camera->removeListener(this);
    }
}

//----------------------------------------------------------------------------
TerrainPatch* TerrainPatch::create(Terrain* terrain,
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
                                   float verticalSkirtSize)
{
    // Create patch
    TerrainPatch* patch = new TerrainPatch();
    patch->_terrain = terrain;
    patch->_index = index;
    patch->_row = row;
    patch->_column = column;

    // Add patch lods
    for (size_t step = 1; step <= maxStep; step *= 2)
    {
        patch->addLOD(heights, width, height, x1, z1, x2, z2, xOffset, zOffset, step, verticalSkirtSize);
    }

    // Set our bounding box using the base LOD mesh
    BoundingBox& bounds = patch->_boundingBox;
    bounds.set(patch->_levels[0]->model->getMesh()->getBoundingBox());

    return patch;
}

//----------------------------------------------------------------------------
Material* TerrainPatch::getMaterial(int index) const
{
    if (index == -1)
    {
        Scene* scene = _terrain->_node ? _terrain->_node->getScene() : nullptr;
        Camera* camera = scene ? scene->getActiveCamera() : nullptr;
        if (!camera)
        {
            _level = const_cast<TerrainPatch*>(this)->computeLOD(camera, getBoundingBox(true));
        }
        else
        {
            _level = 0;
        }
        return _levels[_level]->model->getMaterial();
    }
    return _levels[index]->model->getMaterial();
}

//----------------------------------------------------------------------------
void TerrainPatch::addLOD(float* heights,
                          unsigned int width,
                          unsigned int height,
                          unsigned int x1,
                          unsigned int z1,
                          unsigned int x2,
                          unsigned int z2,
                          float xOffset,
                          float zOffset,
                          unsigned int step,
                          float verticalSkirtSize)
{
    // Allocate vertex data for this patch
    unsigned int patchWidth;
    unsigned int patchHeight;
    if (step == 1)
    {
        patchWidth = (x2 - x1) + 1;
        patchHeight = (z2 - z1) + 1;
    }
    else
    {
        patchWidth = (x2 - x1) / step + ((x2 - x1) % step == 0 ? 0 : 1) + 1;
        patchHeight = (z2 - z1) / step + ((z2 - z1) % step == 0 ? 0 : 1) + 1;
    }

    if (patchWidth < 2 || patchHeight < 2) return; // ignore this level, not enough geometry

    if (verticalSkirtSize > 0.0f)
    {
        patchWidth += 2;
        patchHeight += 2;
    }

    unsigned int vertexCount = patchHeight * patchWidth;
    unsigned int vertexElements = _terrain->_normalMap ? 5 : 8; //<x,y,z>[i,j,k]<u,v>
    std::vector<float> vertices(vertexCount * vertexElements);

    unsigned int index = 0;
    Vector3 min(FLT_MAX, FLT_MAX, FLT_MAX);
    Vector3 max(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    float stepXScaled = step * _terrain->_localScale.x;
    float stepZScaled = step * _terrain->_localScale.z;
    bool zskirt = verticalSkirtSize > 0 ? true : false;
    for (unsigned int z = z1;;)
    {
        bool xskirt = verticalSkirtSize > 0 ? true : false;
        for (unsigned int x = x1;;)
        {
            assert(index < vertexCount);

            float* v = vertices.data() + (index * vertexElements);
            index++;

            // Compute position - apply the local scale of the terrain into the vertex data
            v[0] = (x + xOffset) * _terrain->_localScale.x;
            v[1] = computeHeight(heights, width, x, z);
            if (xskirt || zskirt) v[1] -= verticalSkirtSize * _terrain->_localScale.y;
            v[2] = (z + zOffset) * _terrain->_localScale.z;

            // Update bounding box min/max (don't include vertical skirt vertices in bounding box)
            if (!(xskirt || zskirt))
            {
                if (v[0] < min.x) min.x = v[0];
                if (v[1] < min.y) min.y = v[1];
                if (v[2] < min.z) min.z = v[2];
                if (v[0] > max.x) max.x = v[0];
                if (v[1] > max.y) max.y = v[1];
                if (v[2] > max.z) max.z = v[2];
            }

            // Compute normal
            if (!_terrain->_normalMap)
            {
                Vector3 p(v[0], computeHeight(heights, width, x, z), v[2]);
                Vector3 w(Vector3(x >= step ? v[0] - stepXScaled : v[0],
                                  computeHeight(heights, width, x >= step ? x - step : x, z),
                                  v[2]),
                          p);
                Vector3 e(Vector3(x < width - step ? v[0] + stepXScaled : v[0],
                                  computeHeight(heights, width, x < width - step ? x + step : x, z),
                                  v[2]),
                          p);
                Vector3 s(Vector3(v[0],
                                  computeHeight(heights, width, x, z >= step ? z - step : z),
                                  z >= step ? v[2] - stepZScaled : v[2]),
                          p);
                Vector3 n(Vector3(v[0],
                                  computeHeight(heights, width, x, z < height - step ? z + step : z),
                                  z < height - step ? v[2] + stepZScaled : v[2]),
                          p);
                Vector3 normals[4];
                Vector3::cross(n, w, &normals[0]);
                Vector3::cross(w, s, &normals[1]);
                Vector3::cross(e, n, &normals[2]);
                Vector3::cross(s, e, &normals[3]);
                Vector3 normal = -(normals[0] + normals[1] + normals[2] + normals[3]);
                normal.normalize();
                v[3] = normal.x;
                v[4] = normal.y;
                v[5] = normal.z;
                v += 3;
            }

            v += 3;

            // Compute texture coord
            v[0] = (float)x / (width - 1);
            v[1] = 1.0f - (float)z / (height - 1);
            if (xskirt)
            {
                float offset = verticalSkirtSize / width;
                v[0] = x == x1 ? v[0] - offset : v[0] + offset;
            }
            else if (zskirt)
            {
                float offset = verticalSkirtSize / height;
                v[1] = z == z1 ? v[1] - offset : v[1] + offset;
            }

            if (x == x2)
            {
                if ((verticalSkirtSize == 0) || xskirt)
                    break;
                else
                    xskirt = true;
            }
            else if (xskirt)
                xskirt = false;
            else
                x = std::min(x + step, x2);
        }

        if (z == z2)
        {
            if ((verticalSkirtSize == 0) || zskirt)
                break;
            else
                zskirt = true;
        }
        else if (zskirt)
            zskirt = false;
        else
            z = std::min(z + step, z2);
    }
    assert(index == vertexCount);

    Vector3 center(min + ((max - min) * 0.5f));

    // Create mesh
    VertexFormat::Element elements[3];
    elements[0] = VertexFormat::Element(VertexFormat::POSITION, 3);
    if (_terrain->_normalMap)
    {
        elements[1] = VertexFormat::Element(VertexFormat::TEXCOORD0, 2);
    }
    else
    {
        elements[1] = VertexFormat::Element(VertexFormat::NORMAL, 3);
        elements[2] = VertexFormat::Element(VertexFormat::TEXCOORD0, 2);
    }
    VertexFormat format(elements, _terrain->_normalMap ? 2 : 3);
    auto mesh = Mesh::createMesh(format, vertexCount);
    mesh->setVertexData(vertices.data());
    mesh->setBoundingBox(BoundingBox(min, max));
    mesh->setBoundingSphere(BoundingSphere(center, center.distance(max)));

    // Add mesh part for indices
    unsigned int indexCount = (patchWidth * 2) * // # indices per row of tris
                                  (patchHeight - 1)
                              +                      // # rows of tris
                              (patchHeight - 2) * 2; // # degenerate tris

    // Support a maximum number of indices of USHRT_MAX. Any more indices we will require breaking
    // up the terrain into smaller patches.
    if (indexCount > USHRT_MAX)
    {
        GP_WARN("Index count of %d for terrain patch exceeds the limit of 65535. Please specifiy a "
                "smaller patch size.",
                indexCount);
        assert(indexCount <= USHRT_MAX);
    }

    auto part = mesh->addPart(Mesh::TRIANGLE_STRIP, Mesh::INDEX16, indexCount);
    std::vector<unsigned short> indices(indexCount);
    index = 0;
    for (unsigned int z = 0; z < patchHeight - 1; ++z)
    {
        unsigned int i1 = z * patchWidth;
        unsigned int i2 = (z + 1) * patchWidth;

        // Move left to right for even rows and right to left for odd rows.
        // Note that this results in two degenerate triangles between rows
        // for stitching purposes, but actually does not require any extra
        // indices to achieve this.
        if (z % 2 == 0)
        {
            if (z > 0)
            {
                // Add degenerate indices to connect strips
                indices[index] = indices[index - 1];
                ++index;
                indices[index++] = i1;
            }

            // Add row strip
            for (size_t x = 0; x < patchWidth; ++x)
            {
                indices[index++] = i1 + x;
                indices[index++] = i2 + x;
            }
        }
        else
        {
            // Add degenerate indices to connect strips
            if (z > 0)
            {
                indices[index] = indices[index - 1];
                ++index;
                indices[index++] = i2 + ((int)patchWidth - 1);
            }

            // Add row strip
            for (int x = (int)patchWidth - 1; x >= 0; --x)
            {
                indices[index++] = i2 + x;
                indices[index++] = i1 + x;
            }
        }
    }
    assert(index == indexCount);
    part->setIndexData(indices.data(), 0, indexCount);

    // Add this level - vectors clean up automatically when function exits
    _levels.push_back(std::make_unique<Level>());
    _levels.back()->model = Model::create(mesh);
}

//----------------------------------------------------------------------------
void TerrainPatch::deleteLayer(Layer* layer)
{
    // Release layer samplers - with shared_ptr, just reset the entry
    if (layer->textureIndex != -1 && layer->textureIndex < (int)_samplers.size())
    {
        // Check if this sampler is only referenced by this layer (use_count == 1)
        if (_samplers[layer->textureIndex].use_count() == 1)
        {
            _samplers[layer->textureIndex].reset();
        }
        // Note: If use_count > 1, other layers share this sampler, so we don't reset
    }

    if (layer->blendIndex != -1 && layer->blendIndex < (int)_samplers.size())
    {
        if (_samplers[layer->blendIndex].use_count() == 1)
        {
            _samplers[layer->blendIndex].reset();
        }
    }

    _layers.erase(layer);
    delete layer;
}

//----------------------------------------------------------------------------
int TerrainPatch::addSampler(const std::string& path)
{
    // TODO: Support shared samplers stored in Terrain class for layers that span all patches
    // on the terrain (row == col == -1).

    // Load the texture. If this texture is already loaded, it will return
    // a pointer to the same one, with its ref count incremented.
    TexturePtr texture = Texture::create(path, true);
    if (!texture) return -1;

    // Textures should only be 2D
    if (texture->getType() != Texture::TEXTURE_2D)
    {
        return -1;
    }

    int firstAvailableIndex = -1;
    for (size_t i = 0, count = _samplers.size(); i < count; ++i)
    {
        SamplerPtr& sampler = _samplers[i];

        if (!sampler && firstAvailableIndex == -1)
        {
            firstAvailableIndex = (int)i;
        }
        else if (sampler && sampler->getTexture() == texture.get())
        {
            // A sampler was already added for this texture.
            // With shared_ptr, it's already reference counted.
            return (int)i;
        }
    }

    // Add a new sampler to the list
    SamplerPtr sampler = Texture::Sampler::create(texture);

    // This may need to be clamp in some cases to prevent edge bleeding?  Possibly a
    // configuration variable in the future.
    sampler->setWrapMode(Texture::REPEAT, Texture::REPEAT);
    sampler->setFilterMode(Texture::LINEAR_MIPMAP_LINEAR, Texture::LINEAR);
    if (firstAvailableIndex != -1)
    {
        _samplers[firstAvailableIndex] = sampler;
        return firstAvailableIndex;
    }

    _samplers.push_back(sampler);
    return (int)(_samplers.size() - 1);
}

//----------------------------------------------------------------------------
bool TerrainPatch::setLayer(int index,
                            const std::string& texturePath,
                            const Vector2& textureRepeat,
                            const std::string& blendPath,
                            int blendChannel)
{
    // If there is an existing layer at this index, delete it
    for (auto layer : _layers)
    {
        if (layer->index == index)
        {
            deleteLayer(layer);
            break;
        }
    }

    // Load texture sampler
    int textureIndex = addSampler(texturePath);
    if (textureIndex == -1) return false;

    // Load blend sampler
    int blendIndex = -1;
    if (!blendPath.empty())
    {
        blendIndex = addSampler(blendPath);
    }

    // Create the layer
    Layer* layer = new Layer();
    layer->index = index;
    layer->textureIndex = textureIndex;
    layer->textureRepeat = textureRepeat;
    layer->blendIndex = blendIndex;
    layer->blendChannel = blendChannel;

    _layers.insert(layer);

    _bits |= TERRAINPATCH_DIRTY_MATERIAL;

    return true;
}

std::string TerrainPatch::passCallback(Pass* pass, void* cookie)
{
    TerrainPatch* patch = reinterpret_cast<TerrainPatch*>(cookie);
    assert(patch);

    return patch->passCreated(pass);
}

//----------------------------------------------------------------------------
std::string TerrainPatch::passCreated(Pass* pass)
{
    // Build preprocessor string to be passed to the terrain shader.
    // NOTE: I make heavy use of preprocessor definitions, rather than passing in arrays and doing
    // non-constant array access in the shader. This is due to the fact that non-constant array
    // access in GLES is very slow on some hardware.
    std::ostringstream defines;
    defines << "LAYER_COUNT " << _layers.size();
    defines << ";SAMPLER_COUNT " << _samplers.size();

    if (_terrain->isFlagSet(Terrain::DEBUG_PATCHES))
    {
        defines << ";DEBUG_PATCHES";
        pass->getParameter("u_row")->setFloat(_row);
        pass->getParameter("u_column")->setFloat(_column);
    }

    if (_terrain->_normalMap) defines << ";NORMAL_MAP";

    // Append texture and blend index constants to preprocessor definition.
    // We need to do this since older versions of GLSL only allow sampler arrays
    // to be indexed using constant expressions (otherwise we could simply pass an
    // array of indices to use for sampler lookup).
    //
    // Rebuild layer lists while we're at it.
    //
    int layerIndex = 0;
    for (std::set<Layer*, LayerCompare>::iterator itr = _layers.begin(); itr != _layers.end();
         ++itr, ++layerIndex)
    {
        Layer* layer = *itr;

        defines << ";TEXTURE_INDEX_" << layerIndex << " " << layer->textureIndex;
        defines << ";TEXTURE_REPEAT_" << layerIndex << " vec2(" << layer->textureRepeat.x << ","
                << layer->textureRepeat.y << ")";

        if (layerIndex > 0)
        {
            defines << ";BLEND_INDEX_" << layerIndex << " " << layer->blendIndex;
            defines << ";BLEND_CHANNEL_" << layerIndex << " " << layer->blendChannel;
        }
    }

    return defines.str();
}

//----------------------------------------------------------------------------
bool TerrainPatch::updateMaterial()
{
    if (!(_bits & TERRAINPATCH_DIRTY_MATERIAL)) return true;

    _bits &= ~TERRAINPATCH_DIRTY_MATERIAL;

    __currentPatchIndex = _index;

    for (size_t i = 0, count = _levels.size(); i < count; ++i)
    {
        MaterialPtr material = Material::create(_terrain->_materialPath, &passCallback, this);
        assert(material);
        if (!material)
        {
            GP_WARN("Failed to load material for terrain patch: %s", _terrain->_materialPath.c_str());
            __currentPatchIndex = -1;
            return false;
        }

        material->setNodeBinding(_terrain->_node);

        // Set material on this lod level
        _levels[i]->model->setMaterial(material);
    }

    __currentPatchIndex = -1;

    return true;
}

//----------------------------------------------------------------------------
void TerrainPatch::updateNodeBindings()
{
    __currentPatchIndex = _index;
    for (size_t i = 0, count = _levels.size(); i < count; ++i)
    {
        _levels[i]->model->getMaterial()->setNodeBinding(_terrain->_node);
    }
    __currentPatchIndex = -1;
}

//----------------------------------------------------------------------------
unsigned int TerrainPatch::draw(bool wireframe)
{
    Scene* scene = _terrain->_node ? _terrain->_node->getScene() : nullptr;
    Camera* camera = scene ? scene->getActiveCamera() : nullptr;
    if (!camera) return 0;

    // Get our world-space bounding box
    BoundingBox bounds = getBoundingBox(true);

    // If the box does not intersect the view frustum, cull it
    if (_terrain->isFlagSet(Terrain::FRUSTUM_CULLING) && !camera->getFrustum().intersects(bounds))
        return 0;

    if (!updateMaterial()) return 0;

    // Compute the LOD level from the camera's perspective
    _level = computeLOD(camera, bounds);

    // Draw the model for the current LOD
    return _levels[_level]->model->draw(wireframe);
}

//----------------------------------------------------------------------------
const BoundingBox& TerrainPatch::getBoundingBox(bool worldSpace) const
{
    if (!worldSpace) return _boundingBox;

    if (!(_bits & TERRAINPATCH_DIRTY_BOUNDS)) return _boundingBoxWorld;

    _bits &= ~TERRAINPATCH_DIRTY_BOUNDS;

    // Apply a world-space transformation to our bounding box
    _boundingBoxWorld.set(_boundingBox);

    // Transform the bounding box by the terrain node's world transform.
    if (_terrain->_node) _boundingBoxWorld.transform(_terrain->_node->getWorldMatrix());

    return _boundingBoxWorld;
}

//----------------------------------------------------------------------------
unsigned int TerrainPatch::computeLOD(Camera* camera, const BoundingBox& worldBounds)
{
    auto cachedCamera = _camera.lock();
    if (!cachedCamera || cachedCamera.get() != camera)
    {
        if (cachedCamera)
        {
            cachedCamera->removeListener(this);
        }
        if (camera && camera->getNode())
        {
            _camera = camera->getNode()->_camera;
            if (auto newCamera = _camera.lock())
            {
                newCamera->addListener(this);
            }
        }
        else
        {
            _camera.reset();
        }
        _bits |= TERRAINPATCH_DIRTY_LEVEL;
    }

    // base level
    if (!_terrain->isFlagSet(Terrain::LEVEL_OF_DETAIL) || _levels.size() == 0) return 0;

    if (!(_bits & TERRAINPATCH_DIRTY_LEVEL)) return _level;

    _bits &= ~TERRAINPATCH_DIRTY_LEVEL;

    // Compute LOD to use based on very simple distance metric. TODO: Optimize me.
    Game* game = Game::getInstance();
    Rectangle vp(0, 0, game->getWidth(), game->getHeight());
    Vector3 corners[8];
    Vector2 min(FLT_MAX, FLT_MAX);
    Vector2 max(-FLT_MAX, -FLT_MAX);
    worldBounds.getCorners(corners);
    for (size_t i = 0; i < 8; ++i)
    {
        const Vector3& corner = corners[i];
        float x, y;
        camera->project(vp, corners[i], &x, &y);
        min.x = std::min<float>(min.x, x);
        min.y = std::min<float>(min.y, y);

        max.x = std::max<float>(max.x, x);
        max.y = std::max<float>(max.y, x);
    }
    float area = (max.x - min.x) * (max.y - min.y);
    float screenArea = game->getWidth() * game->getHeight() / 10.0f;
    float error = screenArea / area;

    // Level LOD based on distance from camera
    size_t maxLod = _levels.size() - 1;
    size_t lod = (size_t)error;
    lod = std::max(lod, (size_t)0);
    lod = std::min(lod, maxLod);
    _level = lod;

    return _level;
}

//----------------------------------------------------------------------------
const Vector3& TerrainPatch::getAmbientColor() const
{
    Scene* scene = _terrain->_node ? _terrain->_node->getScene() : nullptr;
    return scene ? scene->getAmbientColor() : Vector3::zero();
}

//----------------------------------------------------------------------------
float TerrainPatch::computeHeight(float* heights, unsigned int width, unsigned int x, unsigned int z)
{
    return heights[z * width + x] * _terrain->_localScale.y;
}

//----------------------------------------------------------------------------
bool TerrainPatch::LayerCompare::operator()(const Layer* lhs, const Layer* rhs) const
{
    return (lhs->index < rhs->index);
}

//----------------------------------------------------------------------------
bool TerrainAutoBindingResolver::resolveAutoBinding(const std::string& autoBinding,
                                                    Node* node,
                                                    MaterialParameter* parameter)
{
    // Local helper functions
    struct HelperFunctions
    {
        static TerrainPatch* getPatch(Node* node)
        {
            Terrain* terrain = dynamic_cast<Terrain*>(node->getDrawable());
            if (terrain)
            {
                if (__currentPatchIndex >= 0 && __currentPatchIndex < (int)terrain->_patches.size())
                {
                    return terrain->_patches[__currentPatchIndex];
                }
            }
            return nullptr;
        }
    };

    if (autoBinding == "TERRAIN_LAYER_MAPS")
    {
        TerrainPatch* patch = HelperFunctions::getPatch(node);
        if (patch && patch->_layers.size() > 0)
        {
            // Build a temporary array of raw pointers from SamplerPtr vector
            std::vector<const Texture::Sampler*> rawSamplers;
            rawSamplers.reserve(patch->_samplers.size());
            for (const auto& sampler : patch->_samplers)
            {
                rawSamplers.push_back(sampler.get());
            }
            // Use setSamplerArray with copy=true to avoid dangling pointer to temporary vector
            parameter->setSamplerArray(rawSamplers.data(), (unsigned int)rawSamplers.size(), true);
        }
        return true;
    }
    else if (autoBinding == "TERRAIN_NORMAL_MAP")
    {
        Terrain* terrain = dynamic_cast<Terrain*>(node->getDrawable());
        if (terrain && terrain->_normalMap) parameter->setValue(terrain->_normalMap.get());
        return true;
    }
    else if (autoBinding == "TERRAIN_ROW")
    {
        TerrainPatch* patch = HelperFunctions::getPatch(node);
        if (patch) parameter->setValue((float)patch->_row);
        return true;
    }
    else if (autoBinding == "TERRAIN_COLUMN")
    {
        TerrainPatch* patch = HelperFunctions::getPatch(node);
        if (patch) parameter->setValue((float)patch->_column);
        return true;
    }

    return false;
}

} // namespace tractor

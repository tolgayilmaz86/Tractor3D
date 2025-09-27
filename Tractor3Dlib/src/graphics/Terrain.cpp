#include "pch.h"

#include "graphics/Terrain.h"

#include "framework/FileSystem.h"
#include "graphics/TerrainPatch.h"
#include "scene/Node.h"

namespace tractor
{

// Default terrain material path
#define TERRAIN_MATERIAL "res/materials/terrain.material"

// The default square size of terrain patches for a terrain that
// does not have an explicitly specified patch size.
static const unsigned int DEFAULT_TERRAIN_PATCH_SIZE = 32;

// The default height of a terrain that does not have an explicitly
// specified terrain size, expressed as a ratio of the average
// of the dimensions of the terrain heightfield:
//
//   heightMax = (image.width + image.height) / 2 * DEFAULT_TERRAIN_HEIGHT_RATIO
//
static const float DEFAULT_TERRAIN_HEIGHT_RATIO = 0.3f;

// Terrain dirty flags
static const unsigned int DIRTY_FLAG_INVERSE_WORLD = 1;

static float getDefaultHeight(unsigned int width, unsigned int height);

Terrain::Terrain()
    : Drawable(), _heightfield(nullptr), _normalMap(nullptr),
      _flags(FRUSTUM_CULLING | LEVEL_OF_DETAIL), _dirtyFlags(DIRTY_FLAG_INVERSE_WORLD)
{
}

Terrain::~Terrain()
{
    for (size_t i = 0, count = _patches.size(); i < count; ++i)
    {
        SAFE_DELETE(_patches[i]);
    }
    SAFE_RELEASE(_normalMap);
    SAFE_RELEASE(_heightfield);
}

Terrain* Terrain::create(const std::string& path) { return create(path, nullptr); }

Terrain* Terrain::create(Properties* properties)
{
    return create(properties->getNamespace(), properties);
}

Terrain* Terrain::create(const std::string& path, Properties* properties)
{
    // Terrain properties
    Properties* p = properties;
    Properties* pTerrain = nullptr;
    bool externalProperties = (p != nullptr);
    HeightField* heightfield = nullptr;
    Vector3 terrainSize;
    int patchSize = 0;
    int detailLevels = 1;
    float skirtScale = 0;
    std::string normalMap;
    std::string materialPath;

    if (!p && !path.empty())
    {
        p = Properties::create(path);
    }

    if (!p)
    {
        GP_WARN("Failed to properties for terrain: %s", path.c_str());
        return nullptr;
    }

    pTerrain = p->getNamespace().length() > 0 ? p : p->getNextNamespace();
    if (pTerrain == nullptr)
    {
        GP_WARN("Invalid terrain definition.");
        if (!externalProperties) SAFE_DELETE(p);
        return nullptr;
    }

    // Read heightmap info
    Properties* pHeightmap = pTerrain->getNamespace("heightmap", true);
    if (pHeightmap)
    {
        // Read heightmap path
        std::string heightmap;
        if (!pHeightmap->getPath("path", &heightmap))
        {
            GP_WARN("No 'path' property supplied in heightmap section of terrain definition: %s",
                    path);
            if (!externalProperties) SAFE_DELETE(p);
            return nullptr;
        }

        const auto ext = FileSystem::getExtension(heightmap);
        if (ext == ".PNG")
        {
            // Read normalized height values from heightmap image
            heightfield = HeightField::createFromImage(heightmap, 0, 1);
        }
        else if (ext == ".RAW" || ext == ".R16")
        {
            // Require additional properties to be specified for RAW files
            Vector2 imageSize;
            if (!pHeightmap->getVector2("size", &imageSize))
            {
                GP_WARN("Invalid or missing 'size' attribute in heightmap defintion of terrain "
                        "definition: %s",
                        path);
                if (!externalProperties) SAFE_DELETE(p);
                return nullptr;
            }

            // Read normalized height values from RAW file
            heightfield = HeightField::createFromRAW(heightmap,
                                                     (unsigned int)imageSize.x,
                                                     (unsigned int)imageSize.y,
                                                     0,
                                                     1);
        }
        else
        {
            // Unsupported heightmap format
            GP_WARN("Unsupported heightmap format ('%s') in terrain definition: %s",
                    heightmap.c_str(),
                    path);
            if (!externalProperties) SAFE_DELETE(p);
            return nullptr;
        }
    }
    else
    {
        // Try to read 'heightmap' as a simple string property
        std::string heightmap;
        if (!pTerrain->getPath("heightmap", &heightmap))
        {
            GP_WARN("No 'heightmap' property supplied in terrain definition: %s", path);
            if (!externalProperties) SAFE_DELETE(p);
            return nullptr;
        }

        std::string ext = FileSystem::getExtension(heightmap);
        if (ext == ".PNG")
        {
            // Read normalized height values from heightmap image
            heightfield = HeightField::createFromImage(heightmap, 0, 1);
        }
        else if (ext == ".RAW" || ext == ".R16")
        {
            GP_WARN("RAW heightmaps must be specified inside a heightmap block with width and "
                    "height properties.");
            if (!externalProperties) SAFE_DELETE(p);
            return nullptr;
        }
        else
        {
            GP_WARN("Unsupported 'heightmap' format ('%s') in terrain definition: %s.",
                    heightmap.c_str(),
                    path);
            if (!externalProperties) SAFE_DELETE(p);
            return nullptr;
        }
    }

    // Read terrain 'size'
    if (pTerrain->exists("size"))
    {
        if (!pTerrain->getVector3("size", &terrainSize))
        {
            GP_WARN("Invalid 'size' value ('%s') in terrain definition: %s",
                    pTerrain->getString("size"),
                    path);
        }
    }

    // Read terrain 'patch size'
    if (pTerrain->exists("patchSize"))
    {
        patchSize = pTerrain->getInt("patchSize");
    }

    // Read terrain 'detailLevels'
    if (pTerrain->exists("detailLevels"))
    {
        detailLevels = pTerrain->getInt("detailLevels");
    }

    // Read 'skirtScale'
    if (pTerrain->exists("skirtScale"))
    {
        skirtScale = pTerrain->getFloat("skirtScale");
    }

    // Read 'normalMap'
    normalMap = pTerrain->getString("normalMap");

    // Read 'material'
    materialPath = pTerrain->getString("material", "");

    if (heightfield == nullptr)
    {
        GP_WARN("Failed to read heightfield heights for terrain definition: %s", path);
        if (!externalProperties) SAFE_DELETE(p);
        return nullptr;
    }

    if (terrainSize.isZero())
    {
        terrainSize.set(heightfield->getColumnCount(),
                        getDefaultHeight(heightfield->getColumnCount(), heightfield->getRowCount()),
                        heightfield->getRowCount());
    }

    if (patchSize <= 0 || patchSize > (int)heightfield->getColumnCount()
        || patchSize > (int)heightfield->getRowCount())
    {
        patchSize = std::min(heightfield->getRowCount(),
                             std::min(heightfield->getColumnCount(), DEFAULT_TERRAIN_PATCH_SIZE));
    }

    if (detailLevels <= 0) detailLevels = 1;

    if (skirtScale < 0) skirtScale = 0;

    // Compute terrain scale
    Vector3 scale(terrainSize.x / (heightfield->getColumnCount() - 1),
                  terrainSize.y,
                  terrainSize.z / (heightfield->getRowCount() - 1));

    // Create terrain
    Terrain* terrain = create(heightfield,
                              scale,
                              (unsigned int)patchSize,
                              (unsigned int)detailLevels,
                              skirtScale,
                              normalMap,
                              materialPath,
                              pTerrain);

    if (!externalProperties) SAFE_DELETE(p);

    return terrain;
}

Terrain* Terrain::create(HeightField* heightfield,
                         const Vector3& scale,
                         unsigned int patchSize,
                         unsigned int detailLevels,
                         float skirtScale,
                         const std::string& normalMapPath,
                         const std::string& materialPath)
{
    return create(heightfield,
                  scale,
                  patchSize,
                  detailLevels,
                  skirtScale,
                  normalMapPath,
                  materialPath,
                  nullptr);
}

Terrain* Terrain::create(HeightField* heightfield,
                         const Vector3& scale,
                         unsigned int patchSize,
                         unsigned int detailLevels,
                         float skirtScale,
                         const std::string& normalMapPath,
                         const std::string& materialPath,
                         Properties* properties)
{
    assert(heightfield);

    unsigned int width = heightfield->getColumnCount();
    unsigned int height = heightfield->getRowCount();

    // Create the terrain object
    Terrain* terrain = new Terrain();
    terrain->_heightfield = heightfield;
    terrain->_materialPath = materialPath.empty() ? TERRAIN_MATERIAL : materialPath;

    // Store terrain local scaling so it can be applied to the heightfield
    terrain->_localScale.set(scale);

    // Store reference to bounding box (it is calculated and updated from TerrainPatch)
    BoundingBox& bounds = terrain->_boundingBox;

    if (!normalMapPath.empty())
    {
        terrain->_normalMap = Texture::Sampler::create(normalMapPath, true);
        terrain->_normalMap->setWrapMode(Texture::CLAMP, Texture::CLAMP);
        assert(terrain->_normalMap->getTexture()->getType() == Texture::TEXTURE_2D);
    }

    float halfWidth = (width - 1) * 0.5f;
    float halfHeight = (height - 1) * 0.5f;

    // Compute the maximum step size, which is a function of our lowest level of detail.
    // This determines how many vertices will be skipped per triange/quad on the lowest
    // level detail terrain patch.
    unsigned int maxStep = (unsigned int)std::pow(2.0, (double)(detailLevels - 1));

    // Create terrain patches
    unsigned int x1, x2, z1, z2;
    unsigned int row = 0, column = 0;
    for (unsigned int z = 0; z < height - 1; z = z2, ++row)
    {
        z1 = z;
        z2 = std::min(z1 + patchSize, height - 1);

        for (unsigned int x = 0; x < width - 1; x = x2, ++column)
        {
            x1 = x;
            x2 = std::min(x1 + patchSize, width - 1);

            // Create this patch
            TerrainPatch* patch = TerrainPatch::create(terrain,
                                                       terrain->_patches.size(),
                                                       row,
                                                       column,
                                                       heightfield->getArray(),
                                                       width,
                                                       height,
                                                       x1,
                                                       z1,
                                                       x2,
                                                       z2,
                                                       -halfWidth,
                                                       -halfHeight,
                                                       maxStep,
                                                       skirtScale);
            terrain->_patches.push_back(patch);

            // Append the new patch's local bounds to the terrain local bounds
            bounds.merge(patch->getBoundingBox(false));
        }
    }

    // Read additional layer information from properties (if specified)
    if (properties)
    {
        // Parse terrain layers
        Properties* lp;
        int index = -1;
        while ((lp = properties->getNextNamespace()) != nullptr)
        {
            if (lp->getNamespace() == "layer")
            {
                // If there is no explicitly specified index for this layer, assume it's the 'next' layer
                if (lp->exists("index"))
                    index = lp->getInt("index");
                else
                    ++index;

                std::string textureMap;
                std::string blendMap;
                Vector2 textureRepeat;
                int blendChannel = 0;
                int row = -1;
                int column = -1;

                // Read layer textures
                Properties* t = lp->getNamespace("texture", true);
                if (t)
                {
                    t->getPath("path", &textureMap);
                    if (!t->getVector2("repeat", &textureRepeat)) textureRepeat.set(1, 1);
                }

                Properties* b = lp->getNamespace("blend", true);
                if (b)
                {
                    b->getPath("path", &blendMap);

                    const std::string& channel = b->getString("channel");
                    if (!channel.empty())
                    {
                        char c = std::toupper(channel[0]);
                        if (c == 'R' || c == '0')
                            blendChannel = 0;
                        else if (c == 'G' || c == '1')
                            blendChannel = 1;
                        else if (c == 'B' || c == '2')
                            blendChannel = 2;
                        else if (c == 'A' || c == '3')
                            blendChannel = 3;
                    }
                }

                // Get patch row/columns that this layer applies to.
                if (lp->exists("row")) row = lp->getInt("row");
                if (lp->exists("column")) column = lp->getInt("column");

                if (!terrain->setLayer(index, textureMap, textureRepeat, blendMap, blendChannel, row, column))
                {
                    GP_WARN("Failed to load terrain layer: %s", textureMap.c_str());
                }
            }
        }
    }

    // Load materials for all patches
    std::ranges::for_each(terrain->_patches, [](const auto& patch) { patch->updateMaterial(); });

    return terrain;
}

void Terrain::setNode(Node* node)
{
    if (_node != node)
    {
        if (_node) _node->removeListener(this);

        Drawable::setNode(node);

        if (_node) _node->addListener(this);

        // Update patch node bindings
        for (size_t i = 0, count = _patches.size(); i < count; ++i)
        {
            _patches[i]->updateNodeBindings();
        }
        _dirtyFlags |= DIRTY_FLAG_INVERSE_WORLD;
    }
}

void Terrain::transformChanged(Transform* transform, long cookie)
{
    _dirtyFlags |= DIRTY_FLAG_INVERSE_WORLD;
}

const Matrix& Terrain::getInverseWorldMatrix() const
{
    if (_dirtyFlags & DIRTY_FLAG_INVERSE_WORLD)
    {
        _dirtyFlags &= ~DIRTY_FLAG_INVERSE_WORLD;

        if (_node)
        {
            _inverseWorldMatrix.set(_node->getWorldMatrix());
        }
        else
        {
            _inverseWorldMatrix = Matrix::identity();
        }
        // Apply local scale and invert
        _inverseWorldMatrix.scale(_localScale);
        _inverseWorldMatrix.invert();
    }
    return _inverseWorldMatrix;
}

bool Terrain::setLayer(int index,
                       const std::string& texturePath,
                       const Vector2& textureRepeat,
                       const std::string& blendPath,
                       int blendChannel,
                       int row,
                       int column)
{
    if (texturePath.empty()) return false;

    // Set layer on applicable patches
    bool result = true;
    for (size_t i = 0, count = _patches.size(); i < count; ++i)
    {
        TerrainPatch* patch = _patches[i];

        if ((row == -1 || (int)patch->_row == row) && (column == -1 || (int)patch->_column == column))
        {
            if (!patch->setLayer(index, texturePath, textureRepeat, blendPath, blendChannel))
                result = false;
        }
    }
    return result;
}

bool Terrain::isFlagSet(Flags flag) const { return (_flags & flag) == flag; }

void Terrain::setFlag(Flags flag, bool on)
{
    bool changed = false;

    if (on)
    {
        if ((_flags & flag) == 0)
        {
            _flags |= flag;
            changed = true;
        }
    }
    else
    {
        if ((_flags & flag) == flag)
        {
            _flags &= ~flag;
            changed = true;
        }
    }

    if (flag == DEBUG_PATCHES && changed)
    {
        // Dirty all materials since they need to be updated to support debug drawing
        for (size_t i = 0, count = _patches.size(); i < count; ++i)
        {
            _patches[i]->setMaterialDirty();
        }
    }
}

unsigned int Terrain::getPatchCount() const { return _patches.size(); }

TerrainPatch* Terrain::getPatch(unsigned int index) const { return _patches[index]; }

const BoundingBox& Terrain::getBoundingBox() const { return _boundingBox; }

float Terrain::getHeight(float x, float z) const
{
    // Calculate the correct x, z position relative to the heightfield data.
    float cols = _heightfield->getColumnCount();
    float rows = _heightfield->getRowCount();

    assert(cols > 0);
    assert(rows > 0);

    // Since the specified coordinates are in world space, we need to use the
    // inverse of our world matrix to transform the world x,z coords back into
    // local heightfield coordinates for indexing into the height array.
    Vector3 v = getInverseWorldMatrix() * Vector3(x, 0.0f, z);
    x = v.x + (cols - 1) * 0.5f;
    z = v.z + (rows - 1) * 0.5f;

    // Get the unscaled height value from the HeightField
    float height = _heightfield->getHeight(x, z);

    // Apply world scale to the height value
    if (_node)
    {
        Vector3 worldScale;
        _node->getWorldMatrix().getScale(&worldScale);
        height *= worldScale.y;
    }

    // Apply local scale
    height *= _localScale.y;

    return height;
}

unsigned int Terrain::draw(bool wireframe)
{
    size_t visibleCount = 0;
    for (size_t i = 0, count = _patches.size(); i < count; ++i)
    {
        visibleCount += _patches[i]->draw(wireframe);
    }
    return visibleCount;
}

Drawable* Terrain::clone(NodeCloneContext& context)
{
    // TODO:
    return nullptr;
}

static float getDefaultHeight(unsigned int width, unsigned int height)
{
    // When terrain height is not specified, we'll use a default height of ~ 0.3 of the image dimensions
    return ((width + height) * 0.5f) * DEFAULT_TERRAIN_HEIGHT_RATIO;
}

} // namespace tractor

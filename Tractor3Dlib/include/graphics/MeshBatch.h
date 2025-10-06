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

#include "graphics/Mesh.h"

namespace tractor
{

class Material;

/**
 * Defines a class for rendering multiple mesh into a single draw call on the graphics device.
 */
class MeshBatch
{
  public:
    /**
     * Creates a new mesh batch.
     *
     * @param vertexFormat The format of vertices in the new batch.
     * @param primitiveType The type of primitives that will be added to the batch.
     * @param materialPath Path to a material file to be used for drawing the batch.
     * @param indexed True if the batched primitives will contain index data, false otherwise.
     * @param initialCapacity The initial capacity of the batch, in triangles.
     * @param growSize Amount to grow the batch by when it overflows (a value of zero prevents batch growing).
     *
     * @return A new mesh batch.
     * @script{create}
     */
    static MeshBatch* create(const VertexFormat& vertexFormat,
                             Mesh::PrimitiveType primitiveType,
                             const std::string& materialPath,
                             bool indexed,
                             unsigned int initialCapacity = 1024,
                             unsigned int growSize = 1024);

    /**
     * Creates a new mesh batch.
     *
     * @param vertexFormat The format of vertices in the new batch.
     * @param primitiveType The type of primitives that will be added to the batch.
     * @param material Material to be used for drawing the batch.
     * @param indexed True if the batched primitives will contain index data, false otherwise.
     * @param initialCapacity The initial capacity of the batch, in triangles.
     * @param growSize Amount to grow the batch by when it overflows (a value of zero prevents batch growing).
     *
     * @return A new mesh batch.
     * @script{create}
     */
    static MeshBatch* create(const VertexFormat& vertexFormat,
                             Mesh::PrimitiveType primitiveType,
                             Material* material,
                             bool indexed,
                             unsigned int initialCapacity = 1024,
                             unsigned int growSize = 1024);

    /**
     * Destructor.
     */
    ~MeshBatch();

    /**
     * Returns the current capacity of the batch.
     *
     * @return The batch capacity.
     */
    unsigned int getCapacity() const noexcept { return _capacity; }

    /**
     * Explicitly sets a new capacity for the batch.
     *
     * @param capacity The new batch capacity.
     */
    void setCapacity(unsigned int capacity);

    /**
     * Returns the material for this mesh batch.
     *
     * @return The material used to draw the batch.
     */
    constexpr Material* getMaterial() const noexcept { return _material; }


    /**
     * Adds a group of primitives to the batch.
     *
     * The vertex list passed in should be a pointer of structs, where the struct T represents
     * the format of a single vertex (e.g. {x,y,z,u,v}).
     *
     * If the batch was created with 'indexed' set to true, then valid index data should be
     * passed in this method. However, if 'indexed' was set to false, the indices and indexCount
     * parameters can be omitted since only vertex data will be used.
     *
     * If the batch created to draw triangle strips, this method assumes that separate calls to
     * add specify separate triangle strips. In this case, this method will automatically stitch
     * separate triangle strips together using degenerate (zero-area) triangles.
     *
     * @param vertices Array of vertices.
     * @param vertexCount Number of vertices.
     * @param indices Array of indices into the vertex array (should be nullptr for non-indexed batches).
     * @param indexCount Number of indices (should be zero for non-indexed batches).
     */
    template <class T>
    void add(const T* vertices,
             unsigned int vertexCount,
             const unsigned short* indices = nullptr,
             unsigned int indexCount = 0)
    {
        add(vertices, sizeof(T), vertexCount, indices, indexCount);
    }

    /**
     * Adds a group of primitives to the batch.
     *
     * The vertex list passed in should be a pointer of floats where every X floats represent a
     * single vertex (e.g. {x,y,z,u,v}).
     *
     * If the batch was created with 'indexed' set to true, then valid index data should be
     * passed in this method. However, if 'indexed' was set to false, the indices and indexCount
     * parameters can be omitted since only vertex data will be used.
     *
     * If the batch created to draw triangle strips, this method assumes that separate calls to
     * add specify separate triangle strips. In this case, this method will automatically stitch
     * separate triangle strips together using degenerate (zero-area) triangles.
     *
     * @param vertices Array of vertices.
     * @param vertexCount Number of vertices.
     * @param indices Array of indices into the vertex array (should be nullptr for non-indexed batches).
     * @param indexCount Number of indices (should be zero for non-indexed batches).
     */
    void add(const float* vertices,
             unsigned int vertexCount,
             const unsigned short* indices = nullptr,
             unsigned int indexCount = 0);

    /**
     * Starts batching.
     *
     * This method should be called before calling add() to add primitives to the batch.
     * After all primitives have been added to the batch, call the finish() method to
     * complete the batch.
     *
     * Calling this method will clear any primitives currently in the batch and set the
     * position of the batch back to the beginning.
     */
    void start() noexcept;

    /**
     * Determines if the batch has been started and not yet finished.
     */
    bool isStarted() const noexcept { return _started; }

    /**
     * Indicates that batching is complete and prepares the batch for drawing.
     */
    void finish() noexcept { _started = false; }

    /**
     * Draws the primitives currently in batch.
     */
    void draw();

  private:
    /**
     * Constructor.
     */
    MeshBatch(const VertexFormat& vertexFormat,
              Mesh::PrimitiveType primitiveType,
              Material* material,
              bool indexed,
              unsigned int initialCapacity,
              unsigned int growSize);

    /**
     * Hidden copy constructor.
     */
    MeshBatch(const MeshBatch& copy);

    /**
     * Hidden copy assignment operator.
     */
    MeshBatch& operator=(const MeshBatch&) = delete;

    void add(const void* vertices,
             size_t size,
             unsigned int vertexCount,
             const unsigned short* indices,
             unsigned int indexCount);

    void updateVertexAttributeBinding();

    bool resize(unsigned int capacity);

    const VertexFormat _vertexFormat;
    Mesh::PrimitiveType _primitiveType;
    Material* _material;
    bool _indexed;
    unsigned int _capacity;
    unsigned int _growSize;
    unsigned int _vertexCapacity;
    unsigned int _indexCapacity;
    unsigned int _vertexCount;
    unsigned int _indexCount;
    unsigned char* _vertices;
    unsigned char* _verticesPtr;
    unsigned short* _indices;
    unsigned short* _indicesPtr;
    bool _started;
};

} // namespace tractor

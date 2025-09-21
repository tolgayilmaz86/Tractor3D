#include "graphics/MeshBatch.h"

namespace tractor
{

  Material* MeshBatch::getMaterial() const
  {
    return _material;
  }

  template <class T>
  void MeshBatch::add(const T* vertices, unsigned int vertexCount, const unsigned short* indices, unsigned int indexCount)
  {
    assert(sizeof(T) == _vertexFormat.getVertexSize());
    add(vertices, sizeof(T), vertexCount, indices, indexCount);
  }

}

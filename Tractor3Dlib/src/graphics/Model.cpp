#include "pch.h"
#include "graphics/Model.h"
#include "graphics/MeshPart.h"
#include "renderer/Technique.h"
#include "renderer/Pass.h"
#include "scene/Node.h"

namespace tractor
{
  Model::Model(std::shared_ptr<Mesh> mesh) : Drawable(),
    _mesh(mesh)
  {
    assert(_mesh);
    _partCount = _mesh->getPartCount();
  }

  Model::~Model()
  {
    SAFE_RELEASE(_material);
    if (_partMaterials)
    {
      for (size_t i = 0; i < _partCount; ++i)
      {
        SAFE_RELEASE(_partMaterials[i]);
      }
      SAFE_DELETE_ARRAY(_partMaterials);
    }
    SAFE_DELETE(_skin);
  }

  Model* Model::create(std::shared_ptr<Mesh> mesh)
  {
    assert(mesh);
    //mesh->addRef();
    return new Model(mesh);
  }

  Mesh* Model::getMesh() const
  {
    return _mesh.get();
  }

  unsigned int Model::getMeshPartCount() const
  {
    assert(_mesh);
    return _mesh->getPartCount();
  }

  Material* Model::getMaterial(int partIndex)
  {
    assert(partIndex == -1 || partIndex >= 0);

    Material* m = nullptr;

    if (partIndex < 0)
      return _material;
    if (partIndex >= (int)_partCount)
      return nullptr;

    // Look up explicitly specified part material.
    if (_partMaterials)
    {
      m = _partMaterials[partIndex];
    }
    if (m == nullptr)
    {
      // Return the shared material.
      m = _material;
    }

    return m;
  }

  void Model::setMaterial(Material* material, int partIndex)
  {
    assert(partIndex == -1 || (partIndex >= 0 && partIndex < (int)getMeshPartCount()));

    Material* oldMaterial = nullptr;

    if (partIndex == -1)
    {
      oldMaterial = _material;

      // Set new shared material.
      if (material)
      {
        _material = material;
        _material->addRef();
      }
    }
    else if (partIndex >= 0 && partIndex < (int)getMeshPartCount())
    {
      // Ensure mesh part count is up-to-date.
      validatePartCount();

      // Release existing part material and part binding.
      if (_partMaterials)
      {
        oldMaterial = _partMaterials[partIndex];
      }
      else
      {
        // Allocate part arrays for the first time.
        if (_partMaterials == nullptr)
        {
          _partMaterials = new Material * [_partCount];
          memset(_partMaterials, 0, sizeof(Material*) * _partCount);
        }
      }

      // Set new part material.
      if (material)
      {
        _partMaterials[partIndex] = material;
        material->addRef();
      }
    }

    // Release existing material and binding.
    if (oldMaterial)
    {
      for (unsigned int i = 0, tCount = oldMaterial->getTechniqueCount(); i < tCount; ++i)
      {
        Technique* t = oldMaterial->getTechniqueByIndex(i);
        assert(t);
        for (unsigned int j = 0, pCount = t->getPassCount(); j < pCount; ++j)
        {
          assert(t->getPassByIndex(j));
          t->getPassByIndex(j)->setVertexAttributeBinding(nullptr);
        }
      }
      SAFE_RELEASE(oldMaterial);
    }

    if (material)
    {
      // Hookup vertex attribute bindings for all passes in the new material.
      for (unsigned int i = 0, tCount = material->getTechniqueCount(); i < tCount; ++i)
      {
        Technique* t = material->getTechniqueByIndex(i);
        assert(t);
        for (unsigned int j = 0, pCount = t->getPassCount(); j < pCount; ++j)
        {
          Pass* p = t->getPassByIndex(j);
          assert(p);
          VertexAttributeBinding* b = VertexAttributeBinding::create(_mesh, p->getEffect());
          p->setVertexAttributeBinding(b);
          SAFE_RELEASE(b);
        }
      }
      // Apply node binding for the new material.
      if (_node)
      {
        setMaterialNodeBinding(material);
      }
    }
  }

  Material* Model::setMaterial(const std::string& vshPath, const std::string& fshPath, const std::string& defines, int partIndex)
  {
    // Try to create a Material with the given parameters.
    Material* material = Material::create(vshPath, fshPath, defines);
    if (material == nullptr)
    {
      GP_ERROR("Failed to create material for model.");
      return nullptr;
    }

    // Assign the material to us.
    setMaterial(material, partIndex);

    // Release the material since we now have a reference to it.
    material->release();

    return material;
  }

  Material* Model::setMaterial(const std::string& materialPath, int partIndex)
  {
    // Try to create a Material from the specified material file.
    Material* material = Material::create(materialPath);
    if (material == nullptr)
    {
      GP_ERROR("Failed to create material for model.");
      return nullptr;
    }

    // Assign the material to us
    setMaterial(material, partIndex);

    // Release the material since we now have a reference to it
    material->release();

    return material;
  }

  bool Model::hasMaterial(unsigned int partIndex) const
  {
    return (partIndex < _partCount && _partMaterials && _partMaterials[partIndex]);
  }

  MeshSkin* Model::getSkin() const
  {
    return _skin;
  }

  void Model::setSkin(MeshSkin* skin)
  {
    if (_skin != skin)
    {
      // Free the old skin
      SAFE_DELETE(_skin);

      // Assign the new skin
      _skin = skin;
      if (_skin)
        _skin->_model = this;
    }
  }

  void Model::setNode(Node* node)
  {
    Drawable::setNode(node);

    // Re-bind node related material parameters
    if (node)
    {
      if (_material)
      {
        setMaterialNodeBinding(_material);
      }
      if (_partMaterials)
      {
        for (unsigned int i = 0; i < _partCount; ++i)
        {
          if (_partMaterials[i])
          {
            setMaterialNodeBinding(_partMaterials[i]);
          }
        }
      }
    }
  }

  static bool drawWireframe(Mesh* mesh)
  {
    switch (mesh->getPrimitiveType())
    {
    case Mesh::TRIANGLES:
    {
      unsigned int vertexCount = mesh->getVertexCount();
      for (unsigned int i = 0; i < vertexCount; i += 3)
      {
        GL_ASSERT(glDrawArrays(GL_LINE_LOOP, i, 3));
      }
    }
    return true;

    case Mesh::TRIANGLE_STRIP:
    {
      unsigned int vertexCount = mesh->getVertexCount();
      for (unsigned int i = 2; i < vertexCount; ++i)
      {
        GL_ASSERT(glDrawArrays(GL_LINE_LOOP, i - 2, 3));
      }
    }
    return true;

    default:
      // not supported
      return false;
    }
  }

  static bool drawWireframe(MeshPart* part)
  {
    unsigned int indexCount = part->getIndexCount();
    unsigned int indexSize = 0;
    switch (part->getIndexFormat())
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
      GP_ERROR("Unsupported index format (%d).", part->getIndexFormat());
      return false;
    }

    switch (part->getPrimitiveType())
    {
    case Mesh::TRIANGLES:
    {
      for (size_t i = 0; i < indexCount; i += 3)
      {
        GL_ASSERT(glDrawElements(GL_LINE_LOOP, 3, part->getIndexFormat(), ((const GLvoid*)(i * indexSize))));
      }
    }
    return true;

    case Mesh::TRIANGLE_STRIP:
    {
      for (size_t i = 2; i < indexCount; ++i)
      {
        GL_ASSERT(glDrawElements(GL_LINE_LOOP, 3, part->getIndexFormat(), ((const GLvoid*)((i - 2) * indexSize))));
      }
    }
    return true;

    default:
      // not supported
      return false;
    }
  }

  unsigned int Model::draw(bool wireframe)
  {
    assert(_mesh);

    unsigned int partCount = _mesh->getPartCount();
    if (partCount == 0)
    {
      // No mesh parts (index buffers).
      if (_material)
      {
        Technique* technique = _material->getTechnique();
        assert(technique);
        unsigned int passCount = technique->getPassCount();
        for (unsigned int i = 0; i < passCount; ++i)
        {
          Pass* pass = technique->getPassByIndex(i);
          assert(pass);
          pass->bind();
          GL_ASSERT(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
          if (!wireframe || !drawWireframe(_mesh.get()))
          {
            GL_ASSERT(glDrawArrays(_mesh->getPrimitiveType(), 0, _mesh->getVertexCount()));
          }
          pass->unbind();
        }
      }
    }
    else
    {
      for (unsigned int i = 0; i < partCount; ++i)
      {
        MeshPart* part = _mesh->getPart(i);
        assert(part);

        // Get the material for this mesh part.
        Material* material = getMaterial(i);
        if (material)
        {
          Technique* technique = material->getTechnique();
          assert(technique);
          unsigned int passCount = technique->getPassCount();
          for (unsigned int j = 0; j < passCount; ++j)
          {
            Pass* pass = technique->getPassByIndex(j);
            assert(pass);
            pass->bind();
            GL_ASSERT(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, part->_indexBuffer));
            if (!wireframe || !drawWireframe(part))
            {
              GL_ASSERT(glDrawElements(part->getPrimitiveType(), part->getIndexCount(), part->getIndexFormat(), 0));
            }
            pass->unbind();
          }
        }
      }
    }
    return partCount;
  }

  void Model::setMaterialNodeBinding(Material* material)
  {
    assert(material);

    if (_node)
    {
      material->setNodeBinding(getNode());
    }
  }

  Drawable* Model::clone(NodeCloneContext& context)
  {
    Model* model = Model::create(_mesh);
    if (!model)
    {
      GP_ERROR("Failed to clone model.");
      return nullptr;
    }

    if (getSkin())
    {
      model->setSkin(getSkin()->clone(context));
    }
    if (getMaterial())
    {
      Material* materialClone = getMaterial()->clone(context);
      if (!materialClone)
      {
        GP_ERROR("Failed to clone material for model.");
        return model;
      }
      model->setMaterial(materialClone);
      materialClone->release();
    }
    if (_partMaterials)
    {
      assert(_partCount == model->_partCount);
      for (unsigned int i = 0; i < _partCount; ++i)
      {
        if (_partMaterials[i])
        {
          Material* materialClone = _partMaterials[i]->clone(context);
          model->setMaterial(materialClone, i);
          materialClone->release();
        }
      }
    }
    return model;
  }

  void Model::validatePartCount()
  {
    assert(_mesh);
    unsigned int partCount = _mesh->getPartCount();

    if (_partCount != partCount)
    {
      // Allocate new arrays and copy old items to them.
      if (_partMaterials)
      {
        Material** oldArray = _partMaterials;
        _partMaterials = new Material * [partCount];
        memset(_partMaterials, 0, sizeof(Material*) * partCount);
        if (oldArray)
        {
          for (unsigned int i = 0; i < _partCount; ++i)
          {
            _partMaterials[i] = oldArray[i];
          }
        }
        SAFE_DELETE_ARRAY(oldArray);
      }
      // Update local part count.
      _partCount = _mesh->getPartCount();
    }
  }

}

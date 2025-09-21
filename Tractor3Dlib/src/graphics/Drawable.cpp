#include "framework/Base.h"
#include "graphics/Drawable.h"
#include "scene/Node.h"


namespace tractor
{

  Drawable::Drawable()
    : _node(nullptr)
  {
  }

  Drawable::~Drawable()
  {
  }

  Node* Drawable::getNode() const
  {
    return _node;
  }

  void Drawable::setNode(Node* node)
  {
    _node = node;
  }

}

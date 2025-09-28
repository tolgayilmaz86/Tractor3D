#include "pch.h"

#include "graphics/Drawable.h"

#include "scene/Node.h"

namespace tractor
{

Drawable::Drawable() : _node(nullptr) {}

Drawable::~Drawable() {}

void Drawable::setNode(Node* node) { _node = node; }

} // namespace tractor

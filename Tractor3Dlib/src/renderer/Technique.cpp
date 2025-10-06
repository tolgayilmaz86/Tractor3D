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

#include "renderer/Technique.h"

#include "renderer/Material.h"
#include "renderer/Pass.h"
#include "scene/Node.h"

namespace tractor
{

Technique::Technique(const std::string& id, Material* material) : _id(id), _material(material)
{
    RenderState::_parent = material;
}

Technique::~Technique()
{
    // Destroy all the passes.
    for (size_t i = 0, count = _passes.size(); i < count; ++i)
    {
        SAFE_RELEASE(_passes[i]);
    }
}

Pass* Technique::getPassByIndex(unsigned int index) const
{
    assert(index < _passes.size());
    return _passes[index];
}

Pass* Technique::getPass(const std::string& id) const
{
    for (size_t i = 0, count = _passes.size(); i < count; ++i)
    {
        Pass* pass = _passes[i];
        assert(pass);
        if (pass->getId() == id)
        {
            return pass;
        }
    }
    return nullptr;
}

void Technique::setNodeBinding(Node* node)
{
    RenderState::setNodeBinding(node);

    for (size_t i = 0, count = _passes.size(); i < count; ++i)
    {
        _passes[i]->setNodeBinding(node);
    }
}

Technique* Technique::clone(Material* material, NodeCloneContext& context) const
{
    Technique* technique = new Technique(getId(), material);
    for (const auto& pass : _passes)
    {
        assert(pass);
        technique->_passes.emplace_back(pass->clone(technique, context));
    }

    RenderState::cloneInto(technique, context);
    technique->_parent = material;
    return technique;
}

} // namespace tractor

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

#include "renderer/Pass.h"

#include "renderer/Material.h"
#include "renderer/Technique.h"
#include "scene/Node.h"

namespace tractor
{

//----------------------------------------------------------------------------
Pass::Pass(const std::string& id, Technique* technique)
    : _id(id), _technique(technique)
{
    RenderState::_parent = _technique;
}

//----------------------------------------------------------------------------
Pass::~Pass()
{
    SAFE_RELEASE(_effect);
    SAFE_RELEASE(_vaBinding);
}

//----------------------------------------------------------------------------
bool Pass::initialize(const std::string& vshPath, const std::string& fshPath, const std::string& defines)
{
    SAFE_RELEASE(_effect);
    SAFE_RELEASE(_vaBinding);

    // Attempt to create/load the effect.
    _effect = Effect::createFromFile(vshPath, fshPath, defines);
    if (_effect == nullptr)
    {
        GP_WARN("Failed to create effect for pass. vertexShader = %s, fragmentShader = %s, defines "
                "= %s",
                vshPath.c_str(),
                fshPath.c_str(),
                defines.c_str());
        return false;
    }

    return true;
}

//----------------------------------------------------------------------------
void Pass::setVertexAttributeBinding(VertexAttributeBinding* binding)
{
    SAFE_RELEASE(_vaBinding);

    if (binding)
    {
        _vaBinding = binding;
        binding->addRef();
    }
}

//----------------------------------------------------------------------------
void Pass::bind()
{
    assert(_effect);

    // Bind our effect.
    _effect->bind();

    // Bind our render state
    RenderState::bind(this);

    // If we have a vertex attribute binding, bind it
    if (_vaBinding)
    {
        _vaBinding->bind();
    }
}

//----------------------------------------------------------------------------
void Pass::unbind()
{
    // If we have a vertex attribute binding, unbind it
    if (_vaBinding)
    {
        _vaBinding->unbind();
    }
}

//----------------------------------------------------------------------------
Pass* Pass::clone(Technique* technique, NodeCloneContext& context) const
{
    assert(_effect);
    _effect->addRef();

    Pass* pass = new Pass(getId(), technique);
    pass->_effect = _effect;

    RenderState::cloneInto(pass, context);
    pass->_parent = technique;
    return pass;
}

} // namespace tractor

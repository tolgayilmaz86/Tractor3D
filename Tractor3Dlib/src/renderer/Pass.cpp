#include "pch.h"

#include "renderer/Pass.h"

#include "renderer/Material.h"
#include "renderer/Technique.h"
#include "scene/Node.h"

namespace tractor
{

Pass::Pass(const std::string& id, Technique* technique)
    : _id(id), _technique(technique), _effect(nullptr), _vaBinding(nullptr)
{
    RenderState::_parent = _technique;
}

// Pass::Pass(const char* id, Technique* technique, Effect* effect) :
//   _id(id ? id : ""), _technique(technique), _effect(effect), _vaBinding(nullptr)
//{
//   RenderState::_parent = _technique;
// }

Pass::~Pass()
{
    SAFE_RELEASE(_effect);
    SAFE_RELEASE(_vaBinding);
}

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

const std::string& Pass::getId() const { return _id; }

Effect* Pass::getEffect() const { return _effect; }

void Pass::setVertexAttributeBinding(VertexAttributeBinding* binding)
{
    SAFE_RELEASE(_vaBinding);

    if (binding)
    {
        _vaBinding = binding;
        binding->addRef();
    }
}

VertexAttributeBinding* Pass::getVertexAttributeBinding() const { return _vaBinding; }

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

void Pass::unbind()
{
    // If we have a vertex attribute binding, unbind it
    if (_vaBinding)
    {
        _vaBinding->unbind();
    }
}

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

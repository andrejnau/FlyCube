#include "Program/ProgramBase.h"
#include <deque>

std::shared_ptr<BindingSet> ProgramBase::CreateBindingSet(const std::vector<BindingDesc>& bindings)
{
    auto it = m_binding_set_cache.find(bindings);
    if (it != m_binding_set_cache.end())
        return it->second;

    auto binding_set = CreateBindingSetImpl(bindings);
    m_binding_set_cache.emplace(bindings, binding_set).first;
    return binding_set;
}

bool ProgramBase::HasShader(ShaderType type) const
{
    return m_shaders_by_type.count(type);
}

const std::shared_ptr<Shader>& ProgramBase::GetShader(ShaderType type) const
{
    return m_shaders_by_type.at(type);
}

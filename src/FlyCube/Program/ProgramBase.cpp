#include "Program/ProgramBase.h"

#include <deque>

ProgramBase::ProgramBase(const std::vector<std::shared_ptr<Shader>>& shaders)
    : m_shaders(shaders)
{
    for (const auto& shader : m_shaders) {
        m_shaders_by_type[shader->GetType()] = shader;
        decltype(auto) bindings = shader->GetBindings();
        m_bindings.insert(m_bindings.begin(), bindings.begin(), bindings.end());

        decltype(auto) reflection = shader->GetReflection();
        decltype(auto) shader_entry_points = reflection->GetEntryPoints();
        m_entry_points.insert(m_entry_points.end(), shader_entry_points.begin(), shader_entry_points.end());
    }
}

bool ProgramBase::HasShader(ShaderType type) const
{
    return m_shaders_by_type.count(type);
}

std::shared_ptr<Shader> ProgramBase::GetShader(ShaderType type) const
{
    auto it = m_shaders_by_type.find(type);
    if (it != m_shaders_by_type.end()) {
        return it->second;
    }
    return {};
}

const std::vector<std::shared_ptr<Shader>>& ProgramBase::GetShaders() const
{
    return m_shaders;
}

const std::vector<BindKey>& ProgramBase::GetBindings() const
{
    return m_bindings;
}

const std::vector<EntryPoint>& ProgramBase::GetEntryPoints() const
{
    return m_entry_points;
}

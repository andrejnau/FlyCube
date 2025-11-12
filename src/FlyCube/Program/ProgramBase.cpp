#include "Program/ProgramBase.h"

#include <deque>

ProgramBase::ProgramBase(const std::vector<std::shared_ptr<Shader>>& shaders)
    : m_shaders(shaders)
{
    for (const auto& shader : m_shaders) {
        m_shaders_by_type[shader->GetType()] = shader;
    }
}

bool ProgramBase::HasShader(ShaderType type) const
{
    return m_shaders_by_type.contains(type);
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

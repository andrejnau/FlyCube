#pragma once
#include "Program/Program.h"

#include <map>
#include <set>
#include <vector>

class ProgramBase : public Program {
public:
    ProgramBase(const std::vector<std::shared_ptr<Shader>>& shaders);

    bool HasShader(ShaderType type) const override final;
    std::shared_ptr<Shader> GetShader(ShaderType type) const override final;
    const std::vector<std::shared_ptr<Shader>>& GetShaders() const override final;
    const std::vector<BindKey>& GetBindings() const override final;
    const std::vector<EntryPoint>& GetEntryPoints() const override final;

protected:
    std::map<ShaderType, std::shared_ptr<Shader>> m_shaders_by_type;
    std::vector<std::shared_ptr<Shader>> m_shaders;
    std::vector<BindKey> m_bindings;
    std::vector<EntryPoint> m_entry_points;
};

#pragma once
#include "Program/Program.h"
#include <set>
#include <map>
#include <vector>

class ProgramBase : public Program
{
public:
    std::shared_ptr<BindingSet> CreateBindingSet(const std::vector<BindingDesc>& bindings) override final;
    bool HasShader(ShaderType type) const override final;
    const std::shared_ptr<Shader>& GetShader(ShaderType type) const override final;

protected:
    virtual std::shared_ptr<BindingSet> CreateBindingSetImpl(const std::vector<BindingDesc>& bindings) = 0;
    std::map<ShaderType, std::shared_ptr<Shader>> m_shaders_by_type;

private:
    std::map<std::vector<BindingDesc>, std::shared_ptr<BindingSet>> m_binding_set_cache;
};

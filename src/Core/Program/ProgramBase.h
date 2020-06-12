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
    using BindingsKey = std::set<size_t>;
    virtual std::shared_ptr<BindingSet> CreateBindingSetImpl(const BindingsKey& bindings) = 0;

    std::vector<BindingDesc> m_bindings;
    std::map<ShaderType, std::shared_ptr<Shader>> m_shaders_by_type;

private:
    std::map<BindingDesc, size_t> m_bindings_id;
    std::map<BindingsKey, std::shared_ptr<BindingSet>> m_binding_set_cache;
};

#pragma once
#include <Instance/QueryInterface.h>
#include <BindingSet/BindingSet.h>
#include <Instance/BaseTypes.h>
#include <Shader/Shader.h>
#include <memory>

class Program : public QueryInterface
{
public:
    virtual ~Program() = default;
    virtual std::shared_ptr<BindingSet> CreateBindingSet(const std::vector<BindingDesc>& bindings) = 0;
    virtual bool HasShader(ShaderType type) const = 0;
    virtual const std::shared_ptr<Shader>& GetShader(ShaderType type) const = 0;
    virtual bool HasBinding(const BindKey& bind_key) const = 0;
};

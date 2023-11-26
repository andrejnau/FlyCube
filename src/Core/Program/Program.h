#pragma once
#include "BindingSet/BindingSet.h"
#include "Instance/BaseTypes.h"
#include "Instance/QueryInterface.h"
#include "Shader/Shader.h"

#include <memory>

class Program : public QueryInterface {
public:
    virtual ~Program() = default;
    virtual bool HasShader(ShaderType type) const = 0;
    virtual std::shared_ptr<Shader> GetShader(ShaderType type) const = 0;
    virtual const std::vector<std::shared_ptr<Shader>>& GetShaders() const = 0;
    virtual const std::vector<BindKey>& GetBindings() const = 0;
    virtual const std::vector<EntryPoint>& GetEntryPoints() const = 0;
};

#pragma once
#include "ShaderReflection/ShaderReflection.h"

#include <spirv_hlsl.hpp>

#include <string>
#include <vector>

BindKey GetBindKey(ShaderType shader_type,
                   const spirv_cross::Compiler& compiler,
                   const spirv_cross::Resource& resource);

class SPIRVReflection : public ShaderReflection {
public:
    SPIRVReflection(const void* data, size_t size);
    const std::vector<EntryPoint>& GetEntryPoints() const override;
    const std::vector<ResourceBindingDesc>& GetBindings() const override;
    const std::vector<VariableLayout>& GetVariableLayouts() const override;
    const std::vector<InputParameterDesc>& GetInputParameters() const override;
    const std::vector<OutputParameterDesc>& GetOutputParameters() const override;
    const ShaderFeatureInfo& GetShaderFeatureInfo() const override;

private:
    std::vector<uint32_t> blob_;
    std::vector<EntryPoint> entry_points_;
    std::vector<ResourceBindingDesc> bindings_;
    std::vector<VariableLayout> layouts_;
    std::vector<InputParameterDesc> input_parameters_;
    std::vector<OutputParameterDesc> output_parameters_;
    ShaderFeatureInfo shader_feature_info_ = {};
};

#pragma once
#include "ShaderReflection/ShaderReflection.h"

#include <spirv_hlsl.hpp>

#include <string>
#include <vector>

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
    std::vector<uint32_t> m_blob;
    std::vector<EntryPoint> m_entry_points;
    std::vector<ResourceBindingDesc> m_bindings;
    std::vector<VariableLayout> m_layouts;
    std::vector<InputParameterDesc> m_input_parameters;
    std::vector<OutputParameterDesc> m_output_parameters;
    ShaderFeatureInfo m_shader_feature_info = {};
};

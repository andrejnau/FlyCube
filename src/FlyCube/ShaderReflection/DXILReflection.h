#pragma once
#include "HLSLCompiler/DXCLoader.h"
#include "ShaderReflection/DXReflection.h"

class DXILReflection : public DXReflection {
public:
    DXILReflection(const void* data, size_t size);
    const std::vector<EntryPoint>& GetEntryPoints() const override;
    const std::vector<ResourceBindingDesc>& GetBindings() const override;
    const std::vector<VariableLayout>& GetVariableLayouts() const override;
    const std::vector<InputParameterDesc>& GetInputParameters() const override;
    const std::vector<OutputParameterDesc>& GetOutputParameters() const override;
    const ShaderFeatureInfo& GetShaderFeatureInfo() const override;

private:
    void ParseRuntimeData(CComPtr<IDxcContainerReflection> reflection, uint32_t idx);
    void ParseDebugInfo(dxc::DxcDllSupport& dxc_support, CComPtr<IDxcBlob> pdb);

    ShaderKind GetVersionShaderType(uint64_t version) override;

    bool m_is_library = false;
    ShaderFeatureInfo m_shader_feature_info = {};
};

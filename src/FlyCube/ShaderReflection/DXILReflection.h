#pragma once
#include "HLSLCompiler/DXCLoader.h"
#include "ShaderReflection/ShaderReflection.h"

#if !defined(_WIN32)
#define interface struct
#endif
#include <directx/d3d12shader.h>

class DXILReflection : public ShaderReflection {
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
    void ParseShaderReflection(CComPtr<ID3D12ShaderReflection> shader_reflection);
    void ParseLibraryReflection(CComPtr<ID3D12LibraryReflection> library_reflection);
    void ParseDebugInfo(dxc::DxcDllSupport& dxc_support, CComPtr<IDxcBlob> pdb);

    bool m_is_library = false;
    std::vector<EntryPoint> m_entry_points;
    std::vector<ResourceBindingDesc> m_bindings;
    std::vector<VariableLayout> m_layouts;
    std::vector<InputParameterDesc> m_input_parameters;
    std::vector<OutputParameterDesc> m_output_parameters;
    ShaderFeatureInfo m_shader_feature_info = {};
};

#pragma once
#include "ShaderReflection/ShaderReflection.h"
#include <HLSLCompiler/DXCLoader.h>
#include <d3d12shader.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXILReflection : public ShaderReflection
{
public:
    DXILReflection(const void* data, size_t size);
    const std::vector<EntryPoint> GetEntryPoints() const override;
    const std::vector<ResourceBindingDesc> GetBindings() const override;
    const std::vector<InputParameterDesc> GetInputParameters() const override;

private:
    void ParseRuntimeData(ComPtr<IDxcContainerReflection> reflection, uint32_t idx);
    void ParseShaderReflection(ComPtr<ID3D12ShaderReflection> shader_reflection);
    void ParseLibraryReflection(ComPtr<ID3D12LibraryReflection> library_reflection);
    void ParseDebugInfo(dxc::DxcDllSupport& dxc_support, ComPtr<IDxcBlob> pdb);

    bool m_is_library = false;
    std::vector<EntryPoint> m_entry_points;
    std::vector<ResourceBindingDesc> m_bindings;
    std::vector<InputParameterDesc> m_input_parameters;
};

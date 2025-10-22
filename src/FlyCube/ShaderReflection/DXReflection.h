#pragma once
#include "HLSLCompiler/DXCLoader.h"
#include "ShaderReflection/ShaderReflection.h"

#if !defined(_WIN32)
#define interface struct
#endif
#include <directx/d3d12shader.h>

class DXReflection : public ShaderReflection {
protected:
    void ParseShaderReflection(CComPtr<ID3D12ShaderReflection> shader_reflection);
    void ParseLibraryReflection(CComPtr<ID3D12LibraryReflection> library_reflection);

    virtual ShaderKind GetVersionShaderType(uint64_t version) = 0;

    std::vector<EntryPoint> m_entry_points;
    std::vector<ResourceBindingDesc> m_bindings;
    std::vector<VariableLayout> m_layouts;
    std::vector<InputParameterDesc> m_input_parameters;
    std::vector<OutputParameterDesc> m_output_parameters;
};

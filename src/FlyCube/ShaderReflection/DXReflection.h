#pragma once
#include "ShaderReflection/ShaderReflection.h"

class ID3D12LibraryReflection;
class ID3D12ShaderReflection;

class DXReflection : public ShaderReflection {
protected:
    void ParseShaderReflection(ID3D12ShaderReflection* shader_reflection);
    void ParseLibraryReflection(ID3D12LibraryReflection* library_reflection);

    virtual ShaderKind GetVersionShaderType(uint64_t version) = 0;

    std::vector<EntryPoint> m_entry_points;
    std::vector<ResourceBindingDesc> m_bindings;
    std::vector<VariableLayout> m_layouts;
    std::vector<InputParameterDesc> m_input_parameters;
    std::vector<OutputParameterDesc> m_output_parameters;
};

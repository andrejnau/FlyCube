#pragma once
#include "ShaderReflection/ShaderReflection.h"

class ID3D12LibraryReflection;
class ID3D12ShaderReflection;
class IDxcContainerReflection;

class DXReflection : public ShaderReflection {
protected:
    void ParseReflectionPart(IDxcContainerReflection* reflection, uint32_t i);
    void ParseShaderReflection(ID3D12ShaderReflection* shader_reflection);
    void ParseLibraryReflection(ID3D12LibraryReflection* library_reflection);

    virtual ShaderKind GetVersionShaderType(uint64_t version) = 0;

    bool m_is_library = false;
    std::vector<EntryPoint> m_entry_points;
    std::vector<ResourceBindingDesc> m_bindings;
    std::vector<VariableLayout> m_layouts;
    std::vector<InputParameterDesc> m_input_parameters;
    std::vector<OutputParameterDesc> m_output_parameters;
};

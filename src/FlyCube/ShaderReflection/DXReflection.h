#pragma once
#include "ShaderReflection/ShaderReflection.h"

class ReflectionPart {
public:
    virtual ~ReflectionPart() = default;
    virtual bool GetShaderReflection(void** ppvObject) const = 0;
    virtual bool GetLibraryReflection(void** ppvObject) const = 0;
};

class DXReflection : public ShaderReflection {
protected:
    virtual ShaderKind GetVersionShaderType(uint64_t version) = 0;
    void ParseReflectionPart(const ReflectionPart& reflection_part);

    bool m_is_library = false;
    std::vector<EntryPoint> m_entry_points;
    std::vector<ResourceBindingDesc> m_bindings;
    std::vector<VariableLayout> m_layouts;
    std::vector<InputParameterDesc> m_input_parameters;
    std::vector<OutputParameterDesc> m_output_parameters;
};

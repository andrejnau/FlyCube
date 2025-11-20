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

    bool is_library_ = false;
    std::vector<EntryPoint> entry_points_;
    std::vector<ResourceBindingDesc> bindings_;
    std::vector<VariableLayout> layouts_;
    std::vector<InputParameterDesc> input_parameters_;
    std::vector<OutputParameterDesc> output_parameters_;
};

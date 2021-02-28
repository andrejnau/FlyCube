#pragma once
#include "ShaderReflection/ShaderReflection.h"
#include <vector>
#include <string>
#include <spirv_hlsl.hpp>

class SPIRVReflection : public ShaderReflection
{
public:
    SPIRVReflection(const void* data, size_t size);
    const std::vector<EntryPoint> GetEntryPoints() const override;
    const std::vector<ResourceBindingDesc> GetBindings() const override;

private:
    ResourceBindingDesc GetBindingDesc(const spirv_cross::CompilerHLSL& compiler, const spirv_cross::Resource& resource);
    void ParseBindings(const spirv_cross::CompilerHLSL& compiler);

    std::vector<uint32_t> m_blob;
    std::vector<EntryPoint> m_entry_points;
    std::vector<ResourceBindingDesc> m_bindings;
};

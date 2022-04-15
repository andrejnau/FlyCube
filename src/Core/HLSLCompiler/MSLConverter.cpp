#include "HLSLCompiler/MSLConverter.h"
#include "HLSLCompiler/Compiler.h"
#include <spirv_msl.hpp>

std::map<std::string, uint32_t> ParseBindings(const spirv_cross::CompilerMSL& compiler)
{
    std::map<std::string, uint32_t> mapping;
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    auto enumerate_resources = [&](const spirv_cross::SmallVector<spirv_cross::Resource>& resources)
    {
        for (const auto& resource : resources)
        {
            std::string name = compiler.get_name(resource.id);
            uint32_t index = compiler.get_automatic_msl_resource_binding(resource.id);
            mapping[name] = index;
        }
    };
    enumerate_resources(resources.uniform_buffers);
    enumerate_resources(resources.storage_buffers);
    enumerate_resources(resources.storage_images);
    enumerate_resources(resources.separate_images);
    enumerate_resources(resources.separate_samplers);
    enumerate_resources(resources.atomic_counters);
    enumerate_resources(resources.acceleration_structures);
    return mapping;
}

std::string GetMSLShader(const std::vector<uint8_t>& blob, std::map<std::string, uint32_t>& mapping)
{
    assert(blob.size() % sizeof(uint32_t) == 0);
    spirv_cross::CompilerMSL compiler((const uint32_t*)blob.data(), blob.size() / sizeof(uint32_t));
    auto options = compiler.get_msl_options();
    // TODO: Fill options
    compiler.set_msl_options(options);
    auto msl_source = compiler.compile();
    mapping = ParseBindings(compiler);
    return msl_source;
}

std::string GetMSLShader(const ShaderDesc& shader)
{
    std::vector<uint8_t> blob = Compile(shader, ShaderBlobType::kSPIRV);
    std::map<std::string, uint32_t> mapping;
    return GetMSLShader(blob, mapping);
}

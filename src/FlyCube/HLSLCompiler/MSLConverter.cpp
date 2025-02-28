#include "HLSLCompiler/MSLConverter.h"

#include "HLSLCompiler/Compiler.h"
#include "ShaderReflection/SPIRVReflection.h"

#include <spirv_msl.hpp>

namespace {

std::map<BindKey, uint32_t> ParseBindings(ShaderType shader_type, const spirv_cross::CompilerMSL& compiler)
{
    std::map<BindKey, uint32_t> mapping;
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    auto enumerate_resources = [&](const spirv_cross::SmallVector<spirv_cross::Resource>& resources) {
        for (const auto& resource : resources) {
            auto bind_key = GetBindKey(shader_type, compiler, resource);
            uint32_t index = compiler.get_automatic_msl_resource_binding(resource.id);
            assert(!mapping.contains(bind_key));
            mapping[bind_key] = index;
        }
    };
    enumerate_resources(resources.uniform_buffers);
    enumerate_resources(resources.storage_buffers);
    enumerate_resources(resources.storage_images);
    enumerate_resources(resources.separate_images);
    enumerate_resources(resources.separate_samplers);
    enumerate_resources(resources.atomic_counters);
    enumerate_resources(resources.acceleration_structures);
    enumerate_resources(resources.push_constant_buffers);
    return mapping;
}

} // namespace

bool UseArgumentBuffers()
{
    return false;
}

std::string GetMSLShader(ShaderType shader_type, const std::vector<uint8_t>& blob, std::map<BindKey, uint32_t>& mapping)
{
    assert(blob.size() % sizeof(uint32_t) == 0);
    spirv_cross::CompilerMSL compiler((const uint32_t*)blob.data(), blob.size() / sizeof(uint32_t));
    auto options = compiler.get_msl_options();
    options.set_msl_version(2, 3);
    options.argument_buffers = UseArgumentBuffers();
    options.force_active_argument_buffer_resources = options.argument_buffers;
    options.argument_buffers_tier = spirv_cross::CompilerMSL::Options::ArgumentBuffersTier::Tier2;
    compiler.set_msl_options(options);
    auto msl_source = compiler.compile();
    mapping = ParseBindings(shader_type, compiler);
    return msl_source;
}

std::string GetMSLShader(const ShaderDesc& shader, std::map<BindKey, uint32_t>& mapping)
{
    std::vector<uint8_t> blob = Compile(shader, ShaderBlobType::kSPIRV);
    return GetMSLShader(shader.type, blob, mapping);
}

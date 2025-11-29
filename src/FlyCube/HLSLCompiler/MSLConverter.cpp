#include "HLSLCompiler/MSLConverter.h"

#include "ShaderReflection/SPIRVReflection.h"
#include "Utilities/Check.h"

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
    CHECK(resources.push_constant_buffers.empty());
    return mapping;
}

} // namespace

std::string GetMSLShader(ShaderType shader_type,
                         const std::vector<uint8_t>& blob,
                         std::map<BindKey, uint32_t>& mapping,
                         std::string& entry_point)
{
    assert(blob.size() % sizeof(uint32_t) == 0);
    spirv_cross::CompilerMSL compiler((const uint32_t*)blob.data(), blob.size() / sizeof(uint32_t));
    auto options = compiler.get_msl_options();
    options.set_msl_version(4, 0);
    options.argument_buffers_tier = spirv_cross::CompilerMSL::Options::ArgumentBuffersTier::Tier2;
    compiler.set_msl_options(options);
    auto msl_source = compiler.compile();
    mapping = ParseBindings(shader_type, compiler);
    auto shader_entry_points = compiler.get_entry_points_and_stages();
    assert(!shader_entry_points.empty());
    entry_point =
        compiler.get_cleansed_entry_point_name(shader_entry_points[0].name, shader_entry_points[0].execution_model);
    return msl_source;
}

#include "Shader/SpirvCompiler.h"
#include <iostream>
#include <fstream>
#include <Utilities/FileUtility.h>
#include <cassert>

#include <shaderc.hpp>

#ifdef _WIN32
#include "Shader/DXCompiler.h"
#endif

std::string ReadShaderFile(const std::string& path)
{
    std::ifstream file(path);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

class SpirvIncludeHandler : public shaderc::CompileOptions::IncluderInterface
{
public:
    SpirvIncludeHandler(const std::string& base_path)
        : m_base_path(base_path)
    {
    }

    virtual shaderc_include_result* GetInclude(const char* requested_source,
        shaderc_include_type type,
        const char* requesting_source,
        size_t include_depth)
    {
        Data* data = new Data{};
        data->path = m_base_path + requested_source;
        data->source = ReadShaderFile(data->path);
        data->res.source_name = data->path.c_str();
        data->res.source_name_length = data->path.size();
        data->res.content = data->source.c_str();
        data->res.content_length = data->source.size();
        data->res.user_data = data;
        return &data->res;
    }

    virtual void ReleaseInclude(shaderc_include_result* res)
    {
        delete static_cast<Data*>(res->user_data);
    }

private:
    std::string m_base_path;
    struct Data
    {
        std::string path;
        std::string source;
        shaderc_include_result res;
    };
};

std::vector<uint32_t> ShadercCompile(const ShaderDesc& shader, const SpirvOption& option)
{
    shaderc_shader_kind shader_type;
    switch (shader.type)
    {
    case ShaderType::kPixel:
        shader_type = shaderc_fragment_shader;
        break;
    case ShaderType::kVertex:
        shader_type = shaderc_vertex_shader;
        break;
    case ShaderType::kGeometry:
        shader_type = shaderc_geometry_shader;
        break;
    case ShaderType::kCompute:
        shader_type = shaderc_compute_shader;
        break;
    default:
        return {};
    }

    shaderc::CompileOptions options;
    for (const auto &x : shader.define)
    {
        options.AddMacroDefinition(x.first, x.second);
    }
    std::string shader_path = GetAssetFullPath(shader.shader_path);
    std::string shader_dir = shader_path.substr(0, shader_path.find_last_of("\\/") + 1);
    options.SetIncluder(std::make_unique<SpirvIncludeHandler>(shader_dir));
    options.SetGenerateDebugInfo();
    options.SetSourceLanguage(shaderc_source_language_hlsl);
    options.SetAutoMapLocations(option.auto_map_bindings);
    options.SetAutoBindUniforms(option.auto_map_bindings);
    options.SetHlslIoMapping(option.hlsl_iomap);
    options.SetHlslFunctionality1(option.fhlsl_functionality1);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    if (option.resource_set_binding != ~0u)
        options.SetHlslRegisterSet(std::to_string(option.resource_set_binding));
    switch (shader.type)
    {
    case ShaderType::kVertex:
    case ShaderType::kGeometry:
        options.SetInvertY(option.invert_y);
        break;
    }
    if (option.vulkan_semantics)
        options.SetTargetEnvironment(shaderc_target_env_vulkan, 0);
    else
        options.SetTargetEnvironment(shaderc_target_env_opengl, 0);

    shaderc::Compiler compiler;
    std::string source = ReadShaderFile(GetAssetFullPath(shader.shader_path));
    shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, shader_type, shader.shader_path.c_str(), shader.entrypoint.c_str(), options);

    if (module.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        std::cerr << module.GetErrorMessage() << std::endl;
        return {};
    }

    return { module.cbegin(), module.cend() };
}

std::vector<uint32_t> SpirvCompile(const ShaderDesc& shader, const SpirvOption& option)
{
#ifdef _WIN32
    if (option.use_dxc)
    {
        DXOption dx_option = { true, option.invert_y };
        auto blob = DXCompile(shader, dx_option);
        if (!blob)
            return {};
        auto blob_as_uint32 = reinterpret_cast<uint32_t*>(blob->GetBufferPointer());
        std::vector<uint32_t> spirv(blob_as_uint32, blob_as_uint32 + blob->GetBufferSize() / 4);
        return spirv;
    }
#endif

    return ShadercCompile(shader, option);
}

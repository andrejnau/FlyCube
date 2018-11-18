#include "Shader/GLSLConverter.h"
#include "Shader/SpirvCompiler.h"
#include <spirv_glsl.hpp>

std::string GetGLSLShader(const ShaderBase& shader)
{
    std::vector<uint32_t> spirv_binary = SpirvCompile(shader);
    spirv_cross::CompilerGLSL glsl(std::move(spirv_binary));
    
    spirv_cross::CompilerGLSL::Options options;
    options.version = 450;
    options.es = false;
    glsl.set_common_options(options);

    glsl.build_dummy_sampler_for_combined_images();
    glsl.build_combined_image_samplers();
    for (auto &remap : glsl.get_combined_image_samplers())
    {
        glsl.set_name(remap.combined_id, spirv_cross::join("SPIRV_Cross_Combined", glsl.get_name(remap.image_id),
            glsl.get_name(remap.sampler_id)));
    }

    std::string source = glsl.compile();

    return source;
}

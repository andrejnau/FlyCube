#include "HLSLCompiler/MSLConverter.h"
#include "HLSLCompiler/Compiler.h"
#include <spirv_msl.hpp>

std::string GetMSLShader(const ShaderDesc& shader)
{
    std::vector<uint8_t> blob = Compile(shader, ShaderBlobType::kSPIRV);
    assert(blob.size() % sizeof(uint32_t) == 0);
    spirv_cross::CompilerMSL compiler((const uint32_t*)blob.data(), blob.size() / sizeof(uint32_t));
    auto options = compiler.get_msl_options();
    // TODO: Fill options
    compiler.set_msl_options(options);
    return compiler.compile();
}

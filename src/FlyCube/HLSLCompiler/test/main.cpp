#include "HLSLCompiler/Compiler.h"
#include "HLSLCompiler/MSLConverter.h"
#include "Utilities/Logging.h"

#include <catch2/catch_all.hpp>

#if defined(__APPLE__)
#import <Metal/Metal.h>
#endif

namespace {

#if defined(__APPLE__)
bool ValidateMSL(const std::string& source)
{
    NSString* ns_source = [NSString stringWithUTF8String:source.c_str()];
    NSError* error = nullptr;
    id<MTLLibrary> library =
        [MTLCreateSystemDefaultDevice() newLibraryWithSource:ns_source options:nullptr error:&error];
    if (!library) {
        Logging::Println("Failed to create MTLLibrary: {}", error);
    }
    return !!library;
}

void CompileToMSL(ShaderType shader_type, const std::vector<uint8_t>& spirv_blob)
{
    if (shader_type == ShaderType::kLibrary) {
        return;
    }

    std::map<BindKey, uint32_t> mapping;
    std::string entry_point;
    auto source = GetMSLShader(shader_type, spirv_blob, mapping, entry_point);
    REQUIRE(!source.empty());
    REQUIRE(ValidateMSL(source));
}
#endif

void RunTest(const ShaderDesc& desc)
{
    SECTION("DXIL")
    {
        auto dxil_blob = Compile(desc, ShaderBlobType::kDXIL);
        REQUIRE(!dxil_blob.empty());
    }

    SECTION("SPIRV")
    {
        auto spirv_blob = Compile(desc, ShaderBlobType::kSPIRV);
        REQUIRE(!spirv_blob.empty());

#if defined(__APPLE__)
        SECTION("MSL")
        {
            CompileToMSL(desc.type, spirv_blob);
        }
#endif
    }
}

} // namespace

TEST_CASE("HLSLCompilerTest")
{
    std::vector<ShaderDesc> shader_descs = {
        { ASSETS_PATH "shaders/BindlessTriangle/PixelShader.hlsl", "main", ShaderType::kPixel, "6_0" },
        { ASSETS_PATH "shaders/BindlessTriangle/VertexShader.hlsl", "main", ShaderType::kVertex, "6_0" },
        { ASSETS_PATH "shaders/DxrTriangle/RayTracing.hlsl", "", ShaderType::kLibrary, "6_3" },
        { ASSETS_PATH "shaders/DxrTriangle/RayTracingCallable.hlsl", "", ShaderType::kLibrary, "6_3" },
        { ASSETS_PATH "shaders/DxrTriangle/RayTracingHit.hlsl", "", ShaderType::kLibrary, "6_3" },
        { ASSETS_PATH "shaders/MeshTriangle/MeshShader.hlsl", "main", ShaderType::kMesh, "6_5" },
        { ASSETS_PATH "shaders/MeshTriangle/PixelShader.hlsl", "main", ShaderType::kPixel, "6_5" },
        { ASSETS_PATH "shaders/Triangle/PixelShader.hlsl", "main", ShaderType::kPixel, "6_0" },
        { ASSETS_PATH "shaders/Triangle/VertexShader.hlsl", "main", ShaderType::kVertex, "6_0" },
    };
    for (const auto& shader_desc : shader_descs) {
        auto test_name = shader_desc.shader_path.substr(std::string_view{ ASSETS_PATH "shaders/" }.size());
        DYNAMIC_SECTION(test_name)
        {
            RunTest(shader_desc);
        }
    }
}

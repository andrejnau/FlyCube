#include "HLSLCompiler/Compiler.h"
#include "HLSLCompiler/MSLConverter.h"

#include <catch2/catch_all.hpp>

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

        SECTION("MSL")
        {
            if (desc.type != ShaderType::kLibrary) {
                std::map<std::string, uint32_t> mapping;
                auto source = GetMSLShader(spirv_blob, mapping);
                REQUIRE(!source.empty());
            }
        }
    }
}

TEST_CASE("HLSLCompilerTest")
{
    std::vector<ShaderDesc> shader_descs = {
        { ASSETS_PATH "shaders/Bindless/PixelShader.hlsl", "main", ShaderType::kPixel, "6_0" },
        { ASSETS_PATH "shaders/Bindless/VertexShader.hlsl", "main", ShaderType::kVertex, "6_0" },
        { ASSETS_PATH "shaders/DxrTriangle/RayTracing.hlsl", "", ShaderType::kLibrary, "6_3" },
        { ASSETS_PATH "shaders/DxrTriangle/RayTracingCallable.hlsl", "", ShaderType::kLibrary, "6_3" },
        { ASSETS_PATH "shaders/DxrTriangle/RayTracingHit.hlsl", "", ShaderType::kLibrary, "6_3" },
        { ASSETS_PATH "shaders/MeshTriangle/MeshShader.hlsl", "main", ShaderType::kMesh, "6_5" },
        { ASSETS_PATH "shaders/MeshTriangle/PixelShader.hlsl", "main", ShaderType::kPixel, "6_5" },
        { ASSETS_PATH "shaders/Triangle/PixelShader.hlsl", "main", ShaderType::kPixel, "6_0" },
        { ASSETS_PATH "shaders/Triangle/PixelShaderNoBindings.hlsl", "main", ShaderType::kPixel, "6_0" },
        { ASSETS_PATH "shaders/Triangle/VertexShader.hlsl", "main", ShaderType::kVertex, "6_0" },
    };
    for (const auto& shader_desc : shader_descs) {
        auto test_name = shader_desc.shader_path.substr(std::string_view{ ASSETS_PATH }.size());
        DYNAMIC_SECTION(test_name)
        {
            RunTest(shader_desc);
        }
    }
}

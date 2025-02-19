#include "HLSLCompiler/Compiler.h"
#include "HLSLCompiler/MSLConverter.h"

#include <catch2/catch_all.hpp>

void RunTest(const ShaderDesc& desc)
{
    auto dxil_blob = Compile(desc, ShaderBlobType::kDXIL);
    REQUIRE(!dxil_blob.empty());

    auto spirv_blob = Compile(desc, ShaderBlobType::kSPIRV);
    REQUIRE(!spirv_blob.empty());

    if (desc.type != ShaderType::kLibrary) {
        std::map<std::string, uint32_t> mapping;
        auto source = GetMSLShader(spirv_blob, mapping);
        REQUIRE(!source.empty());
    }
}

TEST_CASE("TriangleVS")
{
    RunTest({ ASSETS_PATH "shaders/Triangle/VertexShader.hlsl", "main", ShaderType::kVertex, "6_3" });
}

TEST_CASE("TrianglePS")
{
    RunTest({ ASSETS_PATH "shaders/Triangle/PixelShader.hlsl", "main", ShaderType::kPixel, "6_3" });
}

TEST_CASE("RayTracing")
{
    RunTest({ ASSETS_PATH "shaders/DxrTriangle/RayTracing.hlsl", "", ShaderType::kLibrary, "6_3" });
}

TEST_CASE("RayTracingHit")
{
    RunTest({ ASSETS_PATH "shaders/DxrTriangle/RayTracingHit.hlsl", "", ShaderType::kLibrary, "6_3" });
}

TEST_CASE("RayTracingCallable")
{
    RunTest({ ASSETS_PATH "shaders/DxrTriangle/RayTracingCallable.hlsl", "", ShaderType::kLibrary, "6_3" });
}

TEST_CASE("MeshTriangleMS")
{
    RunTest({ ASSETS_PATH "shaders/MeshTriangle/MeshShader.hlsl", "main", ShaderType::kMesh, "6_5" });
}

TEST_CASE("MeshTrianglePS")
{
    RunTest({ ASSETS_PATH "shaders/MeshTriangle/PixelShader.hlsl", "main", ShaderType::kPixel, "6_5" });
}

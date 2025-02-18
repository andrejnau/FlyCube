#include "HLSLCompiler/Compiler.h"
#include "HLSLCompiler/MSLConverter.h"

#include <catch2/catch_all.hpp>

class ShaderTestCase {
public:
    virtual ShaderDesc GetShaderDesc() const = 0;
};

void RunTest(const ShaderTestCase& test_case)
{
    ShaderDesc desc = test_case.GetShaderDesc();

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

class TriangleVS : public ShaderTestCase {
public:
    ShaderDesc GetShaderDesc() const override
    {
        return { ASSETS_PATH "shaders/Triangle/VertexShader.hlsl", "main", ShaderType::kVertex, "6_3" };
    }
};

class TrianglePS : public ShaderTestCase {
public:
    ShaderDesc GetShaderDesc() const override
    {
        return { ASSETS_PATH "shaders/Triangle/PixelShader.hlsl", "main", ShaderType::kPixel, "6_3" };
    }
};

class RayTracing : public ShaderTestCase {
public:
    ShaderDesc GetShaderDesc() const override
    {
        return { ASSETS_PATH "shaders/DxrTriangle/RayTracing.hlsl", "", ShaderType::kLibrary, "6_3" };
    }
};

class RayTracingHit : public ShaderTestCase {
public:
    ShaderDesc GetShaderDesc() const override
    {
        return { ASSETS_PATH "shaders/DxrTriangle/RayTracingHit.hlsl", "", ShaderType::kLibrary, "6_3" };
    }
};

class RayTracingCallable : public ShaderTestCase {
public:
    ShaderDesc GetShaderDesc() const override
    {
        return { ASSETS_PATH "shaders/DxrTriangle/RayTracingCallable.hlsl", "", ShaderType::kLibrary, "6_3" };
    }
};

class MeshTriangleMS : public ShaderTestCase {
public:
    ShaderDesc GetShaderDesc() const override
    {
        return { ASSETS_PATH "shaders/MeshTriangle/MeshShader.hlsl", "main", ShaderType::kMesh, "6_5" };
    }
};

class MeshTrianglePS : public ShaderTestCase {
public:
    ShaderDesc GetShaderDesc() const override
    {
        return { ASSETS_PATH "shaders/MeshTriangle/PixelShader.hlsl", "main", ShaderType::kPixel, "6_5" };
    }
};

TEST_CASE("ShaderReflection")
{
    RunTest(TriangleVS{});
    RunTest(TrianglePS{});
    RunTest(RayTracing{});
    RunTest(RayTracingHit{});
    RunTest(RayTracingCallable{});
    RunTest(MeshTriangleMS{});
    RunTest(MeshTrianglePS{});
}

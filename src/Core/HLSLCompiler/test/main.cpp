#include <HLSLCompiler/Compiler.h>
#include <HLSLCompiler/MSLConverter.h>
#include <catch2/catch_all.hpp>
#include <iostream>

class ShaderTestCase
{
public:
    virtual const ShaderDesc& GetShaderDesc() const = 0;
};

void RunTest(const ShaderTestCase& test_case)
{
#ifdef DIRECTX_SUPPORT
    auto dxil_blob = Compile(test_case.GetShaderDesc(), ShaderBlobType::kDXIL);
    REQUIRE(!dxil_blob.empty());
#endif

#ifdef VULKAN_SUPPORT
    auto spirv_blob = Compile(test_case.GetShaderDesc(), ShaderBlobType::kSPIRV);
    REQUIRE(!spirv_blob.empty());
#endif

#ifdef METAL_SUPPORT
    auto source = GetMSLShader(test_case.GetShaderDesc());
    REQUIRE(!source.empty());
    std::cout << test_case.GetShaderDesc().shader_path << std::endl;
    std::cout << source << std::endl;
#endif
}

class TrianglePS : public ShaderTestCase
{
public:
    const ShaderDesc& GetShaderDesc() const override
    {
        return m_desc;
    }

private:
    ShaderDesc m_desc = { ASSETS_PATH"shaders/CoreTriangle/PixelShader.hlsl", "main", ShaderType::kPixel, "6_3" };
};

class TriangleVS : public ShaderTestCase
{
public:
    const ShaderDesc& GetShaderDesc() const override
    {
        return m_desc;
    }

private:
    ShaderDesc m_desc = { ASSETS_PATH"shaders/CoreTriangle/VertexShader.hlsl", "main", ShaderType::kVertex, "6_3" };
};

TEST_CASE("ShaderReflection")
{
    RunTest(TrianglePS{});
    RunTest(TriangleVS{});
}

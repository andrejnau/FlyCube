#include "HLSLCompiler/Compiler.h"
#include "HLSLCompiler/MSLConverter.h"

#include <catch2/catch_all.hpp>

#include <iostream>

class ShaderTestCase {
public:
    virtual const ShaderDesc& GetShaderDesc() const = 0;
};

void RunTest(const ShaderTestCase& test_case)
{
    auto dxil_blob = Compile(test_case.GetShaderDesc(), ShaderBlobType::kDXIL);
    REQUIRE(!dxil_blob.empty());

    auto spirv_blob = Compile(test_case.GetShaderDesc(), ShaderBlobType::kSPIRV);
    REQUIRE(!spirv_blob.empty());

    std::map<std::string, uint32_t> mapping;
    auto source = GetMSLShader(spirv_blob, mapping);
    REQUIRE(!source.empty());
}

class TrianglePS : public ShaderTestCase {
public:
    const ShaderDesc& GetShaderDesc() const override
    {
        return m_desc;
    }

private:
    ShaderDesc m_desc = { ASSETS_PATH "shaders/CoreTriangle/PixelShader.hlsl", "main", ShaderType::kPixel, "6_3" };
};

class TriangleVS : public ShaderTestCase {
public:
    const ShaderDesc& GetShaderDesc() const override
    {
        return m_desc;
    }

private:
    ShaderDesc m_desc = { ASSETS_PATH "shaders/CoreTriangle/VertexShader.hlsl", "main", ShaderType::kVertex, "6_3" };
};

TEST_CASE("ShaderReflection")
{
    RunTest(TrianglePS{});
    RunTest(TriangleVS{});
}

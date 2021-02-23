#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <ShaderReflection/ShaderReflection.h>
#include <HLSLCompiler/DXCompiler.h>
#include <HLSLCompiler/SpirvCompiler.h>

class ShaderTestCase
{
public:
    virtual const ShaderDesc& GetShaderDesc() const = 0;
    virtual void Test(ShaderBlobType type, const void* data, size_t size) const = 0;
};

void RunTest(const ShaderTestCase& test_case)
{
    auto dxil_blob = DXCompile(test_case.GetShaderDesc());
    REQUIRE(dxil_blob);
    test_case.Test(ShaderBlobType::kDXIL, dxil_blob->GetBufferPointer(), dxil_blob->GetBufferSize());

    auto spirv_blob = SpirvCompile(test_case.GetShaderDesc());
    REQUIRE(spirv_blob.size());
    test_case.Test(ShaderBlobType::kSPIRV, spirv_blob.data(), spirv_blob.size() * sizeof(spirv_blob.front()));
}

class RayTracing : public ShaderTestCase
{
public:
    const ShaderDesc& GetShaderDesc() const override
    {
        return m_desc;
    }

    void Test(ShaderBlobType type, const void* data, size_t size) const
    {
        REQUIRE(data);
        REQUIRE(size);
        auto reflection = CreateShaderReflection(type, data, size);
        REQUIRE(reflection);

        std::vector<EntryPoint> expect = {
            { "ray_gen", ShaderKind::kRayGeneration },
            { "miss",    ShaderKind::kMiss },
            { "closest", ShaderKind::kClosestHit },
        };
        auto entry_points = reflection->GetEntryPoints();
        REQUIRE(entry_points.size() == expect.size());
        for (size_t i = 0; i < entry_points.size(); ++i)
        {
            REQUIRE(entry_points[i].name == expect[i].name);
            REQUIRE(entry_points[i].kind == expect[i].kind);
        }
    }

private:
    ShaderDesc m_desc = { "shaders/DxrTriangle/RayTracing.hlsl", "", ShaderType::kLibrary, "6_5" };
};

class TrianglePS : public ShaderTestCase
{
public:
    const ShaderDesc& GetShaderDesc() const override
    {
        return m_desc;
    }

    void Test(ShaderBlobType type, const void* data, size_t size) const
    {
        REQUIRE(data);
        REQUIRE(size);
        auto reflection = CreateShaderReflection(type, data, size);
        REQUIRE(reflection);

        std::vector<EntryPoint> expect = {
            { "main", ShaderKind::kPixel },
        };
        auto entry_points = reflection->GetEntryPoints();
        REQUIRE(entry_points == expect);
    }

private:
    ShaderDesc m_desc = { "shaders/Triangle/PixelShader_PS.hlsl", "main", ShaderType::kPixel, "6_5" };
};

class TriangleVS : public ShaderTestCase
{
public:
    const ShaderDesc& GetShaderDesc() const override
    {
        return m_desc;
    }

    void Test(ShaderBlobType type, const void* data, size_t size) const
    {
        REQUIRE(data);
        REQUIRE(size);
        auto reflection = CreateShaderReflection(type, data, size);
        REQUIRE(reflection);

        std::vector<EntryPoint> expect = {
            { "main", ShaderKind::kVertex },
        };
        auto entry_points = reflection->GetEntryPoints();
        REQUIRE(entry_points == expect);
    }

private:
    ShaderDesc m_desc = { "shaders/Triangle/VertexShader_VS.hlsl", "main", ShaderType::kVertex, "6_5" };
};

TEST_CASE("ShaderReflection")
{
    RunTest(RayTracing{});
    RunTest(TrianglePS{});
    RunTest(TriangleVS{});
}

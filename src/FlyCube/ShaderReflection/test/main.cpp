#include "HLSLCompiler/Compiler.h"
#include "ShaderReflection/ShaderReflection.h"

#include <catch2/catch_all.hpp>

namespace {

std::shared_ptr<ShaderReflection> CompileAndCreateShaderReflection(const ShaderDesc& desc,
                                                                   ShaderBlobType shader_blob_type)
{
    CAPTURE(shader_blob_type);
    auto blob = Compile(desc, shader_blob_type);
    REQUIRE(!blob.empty());
    auto reflection = CreateShaderReflection(shader_blob_type, blob.data(), blob.size());
    REQUIRE(reflection);
    return reflection;
}

inline auto MakePartialTie(const EntryPoint& self)
{
    return std::tie(self.name, self.kind);
}

} // namespace

inline bool operator==(const EntryPoint& lhs, const EntryPoint& rhs)
{
    return MakePartialTie(lhs) == MakePartialTie(rhs);
}

inline bool operator<(const EntryPoint& lhs, const EntryPoint& rhs)
{
    return MakePartialTie(lhs) < MakePartialTie(rhs);
}

TEST_CASE("RayTracingTriangle/RayTracing.hlsl")
{
    ShaderDesc desc = { ASSETS_PATH "shaders/RayTracingTriangle/RayTracing.hlsl", "", ShaderType::kLibrary, "6_3" };
    auto blob_type = GENERATE(ShaderBlobType::kDXIL, ShaderBlobType::kSPIRV);
    auto reflection = CompileAndCreateShaderReflection(desc, blob_type);

    std::vector<EntryPoint> expect = {
        { "ray_gen", ShaderKind::kRayGeneration },
        { "miss", ShaderKind::kMiss },
    };
    auto entry_points = reflection->GetEntryPoints();
    sort(entry_points.begin(), entry_points.end());
    sort(expect.begin(), expect.end());
    REQUIRE(entry_points.size() == expect.size());
    for (size_t i = 0; i < entry_points.size(); ++i) {
        REQUIRE(entry_points[i].name == expect[i].name);
        REQUIRE(entry_points[i].kind == expect[i].kind);
    }
}

TEST_CASE("Triangle/PixelShader.hlsl")
{
    ShaderDesc desc = { ASSETS_PATH "shaders/Triangle/PixelShader.hlsl", "main", ShaderType::kPixel, "6_0" };
    auto blob_type = GENERATE(ShaderBlobType::kDXIL, ShaderBlobType::kSPIRV);
    auto reflection = CompileAndCreateShaderReflection(desc, blob_type);

    std::vector<EntryPoint> expect = {
        { "main", ShaderKind::kPixel },
    };
    auto entry_points = reflection->GetEntryPoints();
    REQUIRE(entry_points == expect);

    auto bindings = reflection->GetBindings();
    REQUIRE(bindings.size() == 1);
    REQUIRE(bindings.front().name == "constant_buffer");
}

TEST_CASE("Triangle/VertexShader.hlsl")
{
    ShaderDesc desc = { ASSETS_PATH "shaders/Triangle/VertexShader.hlsl", "main", ShaderType::kVertex, "6_0" };
    auto blob_type = GENERATE(ShaderBlobType::kDXIL, ShaderBlobType::kSPIRV);
    auto reflection = CompileAndCreateShaderReflection(desc, blob_type);

    std::vector<EntryPoint> expect = {
        { "main", ShaderKind::kVertex },
    };
    auto entry_points = reflection->GetEntryPoints();
    REQUIRE(entry_points == expect);
}

TEST_CASE("BasicShaderReflectionTest")
{
    std::vector<ShaderDesc> shader_descs = {
        { ASSETS_PATH "shaders/BindlessTriangle/PixelShader.hlsl", "main", ShaderType::kPixel, "6_0" },
        { ASSETS_PATH "shaders/BindlessTriangle/VertexShader.hlsl", "main", ShaderType::kVertex, "6_0" },
        { ASSETS_PATH "shaders/RayTracingTriangle/RayTracing.hlsl", "", ShaderType::kLibrary, "6_3" },
        { ASSETS_PATH "shaders/RayTracingTriangle/RayTracingCallable.hlsl", "", ShaderType::kLibrary, "6_3" },
        { ASSETS_PATH "shaders/RayTracingTriangle/RayTracingHit.hlsl", "", ShaderType::kLibrary, "6_3" },
        { ASSETS_PATH "shaders/MeshTriangle/MeshShader.hlsl", "main", ShaderType::kMesh, "6_5" },
        { ASSETS_PATH "shaders/MeshTriangle/PixelShader.hlsl", "main", ShaderType::kPixel, "6_5" },
        { ASSETS_PATH "shaders/Triangle/PixelShader.hlsl", "main", ShaderType::kPixel, "6_0" },
        { ASSETS_PATH "shaders/Triangle/VertexShader.hlsl", "main", ShaderType::kVertex, "6_0" },
    };
    for (const auto& shader_desc : shader_descs) {
        auto test_name = shader_desc.shader_path.substr(std::string_view{ ASSETS_PATH "shaders/" }.size());
        DYNAMIC_SECTION(test_name)
        {
            auto blob_type = GENERATE(ShaderBlobType::kDXIL, ShaderBlobType::kSPIRV);
            CompileAndCreateShaderReflection(shader_desc, blob_type);
        }
    }
}

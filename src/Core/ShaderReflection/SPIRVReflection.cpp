#include "ShaderReflection/SPIRVReflection.h"
#include <spirv_hlsl.hpp>

ShaderKind ConvertShaderKind(spv::ExecutionModel execution_model)
{
    switch (execution_model)
    {
    case spv::ExecutionModel::ExecutionModelVertex:
        return ShaderKind::kVertex;
    case spv::ExecutionModel::ExecutionModelFragment:
        return ShaderKind::kPixel;
    case spv::ExecutionModel::ExecutionModelGeometry:
        return ShaderKind::kGeometry;
    case spv::ExecutionModel::ExecutionModelGLCompute:
        return ShaderKind::kCompute;
    case spv::ExecutionModel::ExecutionModelRayGenerationNV:
        return ShaderKind::kRayGeneration;
    case spv::ExecutionModel::ExecutionModelIntersectionNV:
        return ShaderKind::kIntersection;
    case spv::ExecutionModel::ExecutionModelAnyHitNV:
        return ShaderKind::kAnyHit;
    case spv::ExecutionModel::ExecutionModelClosestHitNV:
        return ShaderKind::kClosestHit;
    case spv::ExecutionModel::ExecutionModelMissNV:
        return ShaderKind::kMiss;
    case spv::ExecutionModel::ExecutionModelCallableNV:
        return ShaderKind::kCallable;
    case spv::ExecutionModel::ExecutionModelTaskNV:
        return ShaderKind::kAmplification;
    case spv::ExecutionModel::ExecutionModelMeshNV:
        return ShaderKind::kMesh;
    }
    assert(false);
    return ShaderKind::kUnknown;
}

SPIRVReflection::SPIRVReflection(const void* data, size_t size)
    : m_blob((const uint32_t*)data, (const uint32_t*)data + size / sizeof(uint32_t))
{
    spirv_cross::CompilerHLSL compiler(m_blob);
    auto entry_points = compiler.get_entry_points_and_stages();
    for (const auto& entry_point : entry_points)
    {
        m_entry_points.push_back({ entry_point.name.c_str(), ConvertShaderKind(entry_point.execution_model) });
    }
}

const std::vector<EntryPoint> SPIRVReflection::GetEntryPoints() const
{
    return m_entry_points;
}

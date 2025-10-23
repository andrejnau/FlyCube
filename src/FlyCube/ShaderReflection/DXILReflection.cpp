#include "ShaderReflection/DXILReflection.h"

#include "Utilities/DXUtility.h"
#include "Utilities/NotReached.h"

#include <dxc/DxilContainer/DxilContainer.h>
#include <dxc/DxilContainer/DxilRuntimeReflection.inl>
#include <nowide/convert.hpp>

#include <cassert>

#ifdef _WIN32
#include <directx/d3d12shader.h>
#endif

namespace {

ShaderKind ConvertShaderKind(hlsl::DXIL::ShaderKind kind)
{
    switch (kind) {
    case hlsl::DXIL::ShaderKind::Pixel:
        return ShaderKind::kPixel;
    case hlsl::DXIL::ShaderKind::Vertex:
        return ShaderKind::kVertex;
    case hlsl::DXIL::ShaderKind::Geometry:
        return ShaderKind::kGeometry;
    case hlsl::DXIL::ShaderKind::Compute:
        return ShaderKind::kCompute;
    case hlsl::DXIL::ShaderKind::RayGeneration:
        return ShaderKind::kRayGeneration;
    case hlsl::DXIL::ShaderKind::Intersection:
        return ShaderKind::kIntersection;
    case hlsl::DXIL::ShaderKind::AnyHit:
        return ShaderKind::kAnyHit;
    case hlsl::DXIL::ShaderKind::ClosestHit:
        return ShaderKind::kClosestHit;
    case hlsl::DXIL::ShaderKind::Miss:
        return ShaderKind::kMiss;
    case hlsl::DXIL::ShaderKind::Callable:
        return ShaderKind::kCallable;
    case hlsl::DXIL::ShaderKind::Mesh:
        return ShaderKind::kMesh;
    case hlsl::DXIL::ShaderKind::Amplification:
        return ShaderKind::kAmplification;
    default:
        NOTREACHED();
    }
}

} // namespace

DXILReflection::DXILReflection(const void* data, size_t size)
{
    decltype(auto) dxc_support = GetDxcSupport(ShaderBlobType::kDXIL);
    CComPtr<IDxcLibrary> library;
    dxc_support.CreateInstance(CLSID_DxcLibrary, &library);
    CComPtr<IDxcBlobEncoding> source;
    CHECK_HRESULT(library->CreateBlobWithEncodingOnHeapCopy(data, size, CP_ACP, &source));
    CComPtr<IDxcContainerReflection> reflection;
    dxc_support.CreateInstance(CLSID_DxcContainerReflection, &reflection);
    CHECK_HRESULT(reflection->Load(source));

    CComPtr<IDxcBlob> pdb;
    uint32_t part_count = 0;
    CHECK_HRESULT(reflection->GetPartCount(&part_count));
    for (uint32_t i = 0; i < part_count; ++i) {
        uint32_t kind = 0;
        CHECK_HRESULT(reflection->GetPartKind(i, &kind));
        if (kind == hlsl::DxilFourCC::DFCC_RuntimeData) {
            ParseRuntimeData(reflection, i);
        } else if (kind == hlsl::DxilFourCC::DFCC_DXIL) {
            ID3D12ShaderReflection* shader_reflection = nullptr;
            ID3D12LibraryReflection* library_reflection = nullptr;
            if (SUCCEEDED(reflection->GetPartReflection(i, IID_PPV_ARGS(&shader_reflection)))) {
                ParseShaderReflection(shader_reflection);
            } else if (SUCCEEDED(reflection->GetPartReflection(i, IID_PPV_ARGS(&library_reflection)))) {
                m_is_library = true;
                ParseLibraryReflection(library_reflection);
            }
        } else if (kind == hlsl::DxilFourCC::DFCC_ShaderDebugInfoDXIL) {
            CHECK_HRESULT(reflection->GetPartContent(i, &pdb));
        } else if (kind == hlsl::DxilFourCC::DFCC_FeatureInfo) {
            CComPtr<IDxcBlob> part;
            CHECK_HRESULT(reflection->GetPartContent(i, &part));
            assert(part->GetBufferSize() == sizeof(DxilShaderFeatureInfo));
            auto feature_info = reinterpret_cast<DxilShaderFeatureInfo const*>(part->GetBufferPointer());
            if (feature_info->FeatureFlags & hlsl::DXIL::ShaderFeatureInfo_ResourceDescriptorHeapIndexing) {
                m_shader_feature_info.resource_descriptor_heap_indexing = true;
            }
            if (feature_info->FeatureFlags & hlsl::DXIL::ShaderFeatureInfo_SamplerDescriptorHeapIndexing) {
                m_shader_feature_info.sampler_descriptor_heap_indexing = true;
            }
        }
    }

    if (pdb && !m_is_library) {
        ParseDebugInfo(dxc_support, pdb);
    }
}

const std::vector<EntryPoint>& DXILReflection::GetEntryPoints() const
{
    return m_entry_points;
}

const std::vector<ResourceBindingDesc>& DXILReflection::GetBindings() const
{
    return m_bindings;
}

const std::vector<VariableLayout>& DXILReflection::GetVariableLayouts() const
{
    return m_layouts;
}

const std::vector<InputParameterDesc>& DXILReflection::GetInputParameters() const
{
    return m_input_parameters;
}

const std::vector<OutputParameterDesc>& DXILReflection::GetOutputParameters() const
{
    return m_output_parameters;
}

const ShaderFeatureInfo& DXILReflection::GetShaderFeatureInfo() const
{
    return m_shader_feature_info;
}

void DXILReflection::ParseRuntimeData(CComPtr<IDxcContainerReflection> reflection, uint32_t idx)
{
    CComPtr<IDxcBlob> part_blob;
    reflection->GetPartContent(idx, &part_blob);
    hlsl::RDAT::DxilRuntimeData context;
    context.InitFromRDAT(part_blob->GetBufferPointer(), part_blob->GetBufferSize());
    decltype(auto) func_table_reader = context.GetFunctionTable();
    for (uint32_t i = 0; i < func_table_reader.Count(); ++i) {
        decltype(auto) func_reader = func_table_reader[i];
        auto kind = func_reader.getShaderKind();
        m_entry_points.push_back({ func_reader.getUnmangledName(), ConvertShaderKind(kind),
                                   func_reader.getPayloadSizeInBytes(), func_reader.getAttributeSizeInBytes() });
    }
}

void DXILReflection::ParseDebugInfo(dxc::DxcDllSupport& dxc_support, CComPtr<IDxcBlob> pdb)
{
    CComPtr<IDxcPdbUtils2> pdb_utils;
    CHECK_HRESULT(dxc_support.CreateInstance(CLSID_DxcPdbUtils, &pdb_utils));
    CHECK_HRESULT(pdb_utils->Load(pdb));
    CComPtr<IDxcBlobWide> entry_point;
    CHECK_HRESULT(pdb_utils->GetEntryPoint(&entry_point));
    assert(m_entry_points.size() == 1);
    m_entry_points.front().name = nowide::narrow(entry_point->GetStringPointer());
}

ShaderKind DXILReflection::GetVersionShaderType(uint64_t version)
{
    hlsl::DXIL::ShaderKind kind = hlsl::GetVersionShaderType(version);
    return ConvertShaderKind(kind);
}

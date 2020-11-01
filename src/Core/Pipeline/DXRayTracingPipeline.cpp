#include "Pipeline/DXRayTracingPipeline.h"
#include <Device/DXDevice.h>
#include <Program/DXProgram.h>
#include <Shader/DXShader.h>
#include <View/DXView.h>
#include <Utilities/DXGIFormatHelper.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <d3d12shader.h>
#include <Utilities/FileUtility.h>
#include <HLSLCompiler/DXCompiler.h>
#include <HLSLCompiler/DXReflector.h>
#include <HLSLCompiler/DXCLoader.h>
#include <dxc/DXIL/DxilConstants.h>
#include <dxc/DxilContainer/DxilContainer.h>
#include <dxc/DxilContainer/DxilRuntimeReflection.inl>

static const WCHAR* kHitGroup = L"hit_group";

inline uint32_t Align(uint32_t size, uint32_t alignment)
{
    return (size + (alignment - 1)) & ~(alignment - 1);
}

class Subobject
{
public:
    const D3D12_STATE_SUBOBJECT& GetSubobject() const
    {
        return m_state_subobject;
    }

protected:
    D3D12_STATE_SUBOBJECT m_state_subobject = {};
};

struct ShaderInfo
{
    uint32_t max_attribute_size = 0;
    uint32_t max_payload_size = 0;
    std::vector<std::pair<std::wstring, hlsl::DXIL::ShaderKind>> entries;
};

ShaderInfo GetShaderInfo(ComPtr<ID3DBlob> blob)
{
    ShaderInfo res;

    DXCLoader loader;
    CComPtr<IDxcBlobEncoding> source;
    uint32_t shade_idx = 0;
    ASSERT_SUCCEEDED(loader.library->CreateBlobWithEncodingOnHeapCopy(blob->GetBufferPointer(), static_cast<UINT32>(blob->GetBufferSize()), CP_ACP, &source));
    ASSERT_SUCCEEDED(loader.reflection->Load(source));
    uint32_t part_count = 0;
    ASSERT_SUCCEEDED(loader.reflection->GetPartCount(&part_count));
    for (uint32_t i = 0; i < part_count; ++i)
    {
        uint32_t kind = 0;
        ASSERT_SUCCEEDED(loader.reflection->GetPartKind(i, &kind));
        if (kind == hlsl::DxilFourCC::DFCC_RuntimeData)
        {
            CComPtr<IDxcBlob> part_blob;
            loader.reflection->GetPartContent(i, &part_blob);
            hlsl::RDAT::DxilRuntimeData context;
            context.InitFromRDAT(part_blob->GetBufferPointer(), part_blob->GetBufferSize());
            FunctionTableReader* func_table_reader = context.GetFunctionTableReader();
            for (uint32_t j = 0; j < func_table_reader->GetNumFunctions(); ++j)
            {
                FunctionReader func_reader = func_table_reader->GetItem(j);
                std::wstring name = utf8_to_wstring(func_reader.GetUnmangledName());
                hlsl::DXIL::ShaderKind type = func_reader.GetShaderKind();
                res.entries.emplace_back(name, type);

                res.max_attribute_size = std::max(res.max_attribute_size, func_reader.GetAttributeSizeInBytes());
                res.max_payload_size = std::max(res.max_payload_size, func_reader.GetPayloadSizeInBytes());
            }
        }
    }

    return res;
}

std::vector<std::pair<std::wstring, hlsl::DXIL::ShaderKind>> GetEntries(ComPtr<ID3DBlob> blob)
{
    return GetShaderInfo(blob).entries;
}

class DxilLibrary : public Subobject
{
public:
    DxilLibrary(ComPtr<ID3DBlob> blob)
    {
        m_state_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        m_state_subobject.pDesc = &m_dxil_lib_desc;

        auto entries = GetEntries(blob);

        m_export_desc.resize(entries.size());
        m_export_name.resize(entries.size());

        m_dxil_lib_desc.DXILLibrary.pShaderBytecode = blob->GetBufferPointer();
        m_dxil_lib_desc.DXILLibrary.BytecodeLength = blob->GetBufferSize();
        m_dxil_lib_desc.NumExports = entries.size();
        m_dxil_lib_desc.pExports = m_export_desc.data();

        for (size_t i = 0; i < entries.size(); ++i)
        {
            m_export_name[i] = entries[i].first;
            m_export_desc[i].Name = m_export_name[i].c_str();
        }
    }

private:
    D3D12_DXIL_LIBRARY_DESC m_dxil_lib_desc = {};
    std::vector<D3D12_EXPORT_DESC> m_export_desc;
    std::vector<std::wstring> m_export_name;
};

class ExportAssociation : public Subobject
{
public:
    ExportAssociation(const std::vector<std::wstring>& exports, const D3D12_STATE_SUBOBJECT& subobject_to_associate)
    {
        for (auto& str : exports)
        {
            m_exports.emplace_back(str.c_str());
        }

        m_association.NumExports = m_exports.size();
        m_association.pExports = m_exports.data();
        m_association.pSubobjectToAssociate = &subobject_to_associate;

        m_state_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        m_state_subobject.pDesc = &m_association;
    }

private:
    std::vector<const wchar_t*> m_exports;
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION m_association = {};
};

class ShaderConfig : public Subobject
{
public:
    ShaderConfig(ComPtr<ID3DBlob> blob)
    {
        auto info = GetShaderInfo(blob);
        m_shader_config.MaxAttributeSizeInBytes = info.max_attribute_size;
        m_shader_config.MaxPayloadSizeInBytes = info.max_payload_size;
        m_state_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
        m_state_subobject.pDesc = &m_shader_config;
    }

private:
    D3D12_RAYTRACING_SHADER_CONFIG m_shader_config = {};
};

class PipelineConfig : public Subobject
{
public:
    PipelineConfig(uint32_t max_trace_recursion_depth)
    {
        config.MaxTraceRecursionDepth = max_trace_recursion_depth;

        m_state_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
        m_state_subobject.pDesc = &config;
    }

private:
    D3D12_RAYTRACING_PIPELINE_CONFIG config = {};
};

class HitProgram : public Subobject
{
public:
    HitProgram(const wchar_t* hit_group_name, const std::vector<std::pair<std::wstring, hlsl::DXIL::ShaderKind>>& entries)
    {
        for (size_t i = 0; i < entries.size(); ++i)
        {
            switch (entries[i].second)
            {
            case hlsl::DXIL::ShaderKind::Intersection:
                m_desc.IntersectionShaderImport = entries[i].first.c_str();
                break;
            case hlsl::DXIL::ShaderKind::AnyHit:
                m_desc.AnyHitShaderImport = entries[i].first.c_str();
                break;
            case hlsl::DXIL::ShaderKind::ClosestHit:
                m_desc.ClosestHitShaderImport = entries[i].first.c_str();
                break;
            }
        }

        m_desc.HitGroupExport = hit_group_name;

        m_state_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
        m_state_subobject.pDesc = &m_desc;
    }

private:
    D3D12_HIT_GROUP_DESC m_desc = {};
};

DXRayTracingPipeline::DXRayTracingPipeline(DXDevice& device, const RayTracingPipelineDesc& desc)
    : m_device(device)
    , m_desc(desc)
{
    decltype(auto) dx_program = m_desc.program->As<DXProgram>();
    auto shaders = dx_program.GetShaders();
    m_root_signature = dx_program.GetRootSignature();
    auto blob = shaders.front()->GetBlob();

    size_t max_size = 16;
    std::vector<D3D12_STATE_SUBOBJECT> subobjects;
    subobjects.reserve(max_size);

    auto entries = GetEntries(blob);

    std::vector<std::wstring> shader_entries;
    std::vector<std::wstring> hit_group;
    for (size_t i = 0; i < entries.size(); ++i)
    {
        switch (entries[i].second)
        {
        case hlsl::DXIL::ShaderKind::RayGeneration:
        case hlsl::DXIL::ShaderKind::Miss:
            shader_entries.emplace_back(entries[i].first);
            break;
        case hlsl::DXIL::ShaderKind::Intersection:
        case hlsl::DXIL::ShaderKind::AnyHit:
        case hlsl::DXIL::ShaderKind::ClosestHit:
            hit_group.emplace_back(entries[i].first);
            break;
        }
    }

    if (!hit_group.empty())
        shader_entries.emplace_back(kHitGroup);

    DxilLibrary dxil_lib(blob);
    subobjects.emplace_back(dxil_lib.GetSubobject());

    D3D12_STATE_SUBOBJECT& global_root_sign_subobject = subobjects.emplace_back();
    global_root_sign_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    global_root_sign_subobject.pDesc = m_root_signature.GetAddressOf();

    ShaderConfig shader_config(blob);
    subobjects.emplace_back(shader_config.GetSubobject());

    ExportAssociation shader_config_association(shader_entries, subobjects.back());
    subobjects.emplace_back(shader_config_association.GetSubobject());

    PipelineConfig pipeline_config(1);
    subobjects.emplace_back(pipeline_config.GetSubobject());

    HitProgram hit_program(kHitGroup, entries);
    subobjects.emplace_back(hit_program.GetSubobject());

    ASSERT(subobjects.capacity() == max_size);

    D3D12_STATE_OBJECT_DESC ray_tracing_desc = {};
    ray_tracing_desc.NumSubobjects = subobjects.size();
    ray_tracing_desc.pSubobjects = subobjects.data();
    ray_tracing_desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

    ComPtr<ID3D12Device5> device5;
    m_device.GetDevice().As(&device5);
    ASSERT_SUCCEEDED(device5->CreateStateObject(&ray_tracing_desc, IID_PPV_ARGS(&m_pipeline_state)));

    CreateShaderTable();
}

void DXRayTracingPipeline::CreateShaderTable()
{
    decltype(auto) dx_program = m_desc.program->As<DXProgram>();
    auto shaders = dx_program.GetShaders();
    auto blob = shaders.front()->GetBlob();

    auto entries = GetEntries(blob);

    std::vector<std::pair<std::wstring, std::reference_wrapper<D3D12_GPU_VIRTUAL_ADDRESS>>> shader_entries;
    bool has_hit_group = false;
    for (size_t i = 0; i < entries.size(); ++i)
    {
        switch (entries[i].second)
        {
        case hlsl::DXIL::ShaderKind::RayGeneration:
            shader_entries.emplace_back(entries[i].first, m_raytrace_desc.RayGenerationShaderRecord.StartAddress);
            break;
        case hlsl::DXIL::ShaderKind::Miss:
            shader_entries.emplace_back(entries[i].first, m_raytrace_desc.MissShaderTable.StartAddress);
            break;
        case hlsl::DXIL::ShaderKind::Intersection:
        case hlsl::DXIL::ShaderKind::AnyHit:
        case hlsl::DXIL::ShaderKind::ClosestHit:
            has_hit_group = true;
            break;
        }
    }

    if (has_hit_group)
    {
        shader_entries.emplace_back(kHitGroup, m_raytrace_desc.HitGroupTable.StartAddress);
    }

    size_t shader_table_entry_size = Align(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
    shader_table_entry_size = Align(shader_table_entry_size, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    m_device.GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(shader_table_entry_size * shader_entries.size()),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_shader_table)
    );

    uint8_t* shader_table_data = nullptr;
    ASSERT_SUCCEEDED(m_shader_table->Map(0, nullptr, reinterpret_cast<void**>(&shader_table_data)));
    ComPtr<ID3D12StateObjectProperties> state_ojbect_props;
    m_pipeline_state.As(&state_ojbect_props);

    for (size_t i = 0; i < shader_entries.size(); ++i)
    {
        memcpy(shader_table_data + i * shader_table_entry_size, state_ojbect_props->GetShaderIdentifier(shader_entries[i].first.c_str()), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        shader_entries[i].second.get() = m_shader_table->GetGPUVirtualAddress() + i * shader_table_entry_size;
    }

    m_shader_table->Unmap(0, nullptr);

    m_raytrace_desc.RayGenerationShaderRecord.SizeInBytes = shader_table_entry_size;
    m_raytrace_desc.MissShaderTable.SizeInBytes = shader_table_entry_size;
    m_raytrace_desc.HitGroupTable.SizeInBytes = shader_table_entry_size;

    m_raytrace_desc.MissShaderTable.StrideInBytes = shader_table_entry_size;
    m_raytrace_desc.HitGroupTable.StrideInBytes = shader_table_entry_size;
}

PipelineType DXRayTracingPipeline::GetPipelineType() const
{
    return PipelineType::kRayTracing;
}

const RayTracingPipelineDesc& DXRayTracingPipeline::GetDesc() const
{
    return m_desc;
}

const ComPtr<ID3D12StateObject>& DXRayTracingPipeline::GetPipeline() const
{
    return m_pipeline_state;
}

const ComPtr<ID3D12RootSignature>& DXRayTracingPipeline::GetRootSignature() const
{
    return m_root_signature;
}

const D3D12_DISPATCH_RAYS_DESC& DXRayTracingPipeline::GetDispatchRaysDesc() const
{
    return m_raytrace_desc;
}

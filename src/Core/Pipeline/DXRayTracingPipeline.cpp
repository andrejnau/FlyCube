#include "Pipeline/DXRayTracingPipeline.h"
#include <Device/DXDevice.h>
#include <Program/DXProgram.h>
#include <Shader/Shader.h>
#include <BindingSetLayout/DXBindingSetLayout.h>
#include <View/DXView.h>
#include <Utilities/DXGIFormatHelper.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <d3d12shader.h>
#include <Utilities/FileUtility.h>
#include <HLSLCompiler/DXCLoader.h>
#include <dxc/DXIL/DxilConstants.h>
#include <dxc/DxilContainer/DxilContainer.h>
#include <dxc/DxilContainer/DxilRuntimeReflection.h>

static const WCHAR* kHitGroup = L"hit_group";

inline uint32_t Align(uint32_t size, uint32_t alignment)
{
    return (size + (alignment - 1)) & ~(alignment - 1);
}

DXRayTracingPipeline::DXRayTracingPipeline(DXDevice& device, const RayTracingPipelineDesc& desc)
    : m_device(device)
    , m_desc(desc)
{
    decltype(auto) shaders = m_desc.program->GetShaders();
    decltype(auto) dx_layout = m_desc.layout->As<DXBindingSetLayout>();
    m_root_signature = dx_layout.GetRootSignature();

    CD3DX12_STATE_OBJECT_DESC subobjects(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

    decltype(auto) entry_points = m_desc.program->GetEntryPoints();
    for (const auto& shader : shaders)
    {
        decltype(auto) library = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
        decltype(auto) blob = shader->GetBlob();
        D3D12_SHADER_BYTECODE byte = { blob.data(), blob.size() };
        library->SetDXILLibrary(&byte);
    }

    std::vector<std::wstring> shader_entries;
    std::vector<std::wstring> hit_group_entries;
    for (size_t i = 0; i < entry_points.size(); ++i)
    {
        decltype(auto) name = utf8_to_wstring(entry_points[i].name);
        switch (entry_points[i].kind)
        {
        case ShaderKind::kRayGeneration:
        case ShaderKind::kMiss:
            shader_entries.emplace_back(name);
            break;
        case ShaderKind::kIntersection:
        case ShaderKind::kAnyHit:
        case ShaderKind::kClosestHit:
            hit_group_entries.emplace_back(name);
            break;
        }
    }

    if (!hit_group_entries.empty())
    {
        shader_entries.emplace_back(kHitGroup);
    }

    decltype(auto) global_root_signature = subobjects.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    global_root_signature->SetRootSignature(m_root_signature.Get());

    decltype(auto) shader_config = subobjects.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();

    uint32_t max_payload_size = 0;
    uint32_t max_attribute_size = 0;
    for (size_t i = 0; i < entry_points.size(); ++i)
    {
        max_payload_size = std::max(max_payload_size, entry_points[i].payload_size);
        max_attribute_size = std::max(max_payload_size, entry_points[i].attribute_size);
    }
    shader_config->Config(max_payload_size, max_attribute_size);

    decltype(auto) pipeline_config = subobjects.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    pipeline_config->Config(1);

    decltype(auto) hit_group = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    hit_group->SetHitGroupExport(kHitGroup);
    for (size_t i = 0; i < entry_points.size(); ++i)
    {
        decltype(auto) name = utf8_to_wstring(entry_points[i].name);
        switch (entry_points[i].kind)
        {
        case ShaderKind::kIntersection:
            hit_group->SetIntersectionShaderImport(name.c_str());
            break;
        case ShaderKind::kAnyHit:
            hit_group->SetAnyHitShaderImport(name.c_str());
            break;
        case ShaderKind::kClosestHit:
            hit_group->SetClosestHitShaderImport(name.c_str());
            break;
        }
    }

    ComPtr<ID3D12Device5> device5;
    m_device.GetDevice().As(&device5);
    ASSERT_SUCCEEDED(device5->CreateStateObject(subobjects, IID_PPV_ARGS(&m_pipeline_state)));

    CreateShaderTable();
}

void DXRayTracingPipeline::CreateShaderTable()
{
    auto shaders = m_desc.program->GetShaders();
    decltype(auto) entry_points = m_desc.program->GetEntryPoints();

    std::vector<std::pair<std::wstring, std::reference_wrapper<D3D12_GPU_VIRTUAL_ADDRESS>>> shader_entries;
    bool has_hit_group = false;
    for (size_t i = 0; i < entry_points.size(); ++i)
    {
        decltype(auto) name = utf8_to_wstring(entry_points[i].name);
        switch (entry_points[i].kind)
        {
        case ShaderKind::kRayGeneration:
            shader_entries.emplace_back(name.c_str(), m_raytrace_desc.RayGenerationShaderRecord.StartAddress);
            break;
        case ShaderKind::kMiss:
            shader_entries.emplace_back(name.c_str(), m_raytrace_desc.MissShaderTable.StartAddress);
            break;
        case ShaderKind::kIntersection:
        case ShaderKind::kAnyHit:
        case ShaderKind::kClosestHit:
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

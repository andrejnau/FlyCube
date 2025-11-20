#include "Pipeline/DXRayTracingPipeline.h"

#include "BindingSetLayout/DXBindingSetLayout.h"
#include "Device/DXDevice.h"
#include "Shader/Shader.h"
#include "Utilities/DXGIFormatHelper.h"
#include "Utilities/SystemUtils.h"
#include "View/DXView.h"

#include <directx/d3dx12.h>
#include <nowide/convert.hpp>

DXRayTracingPipeline::DXRayTracingPipeline(DXDevice& device, const RayTracingPipelineDesc& desc)
    : device_(device)
    , desc_(desc)
{
    decltype(auto) dx_layout = desc_.layout->As<DXBindingSetLayout>();
    root_signature_ = dx_layout.GetRootSignature();

    CD3DX12_STATE_OBJECT_DESC subobjects(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

    std::vector<EntryPoint> entry_points;
    for (const auto& shader : desc_.shaders) {
        decltype(auto) reflection = shader->GetReflection();
        decltype(auto) shader_entry_points = reflection->GetEntryPoints();
        entry_points.insert(entry_points.end(), shader_entry_points.begin(), shader_entry_points.end());

        decltype(auto) library = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
        decltype(auto) blob = shader->GetBlob();
        D3D12_SHADER_BYTECODE shader_bytecode = { blob.data(), blob.size() };
        library->SetDXILLibrary(&shader_bytecode);
        for (const auto& entry_point : shader_entry_points) {
            uint64_t shader_id = shader->GetId(entry_point.name);
            std::wstring shader_name = nowide::widen(entry_point.name);
            if (shader_names_.contains(shader_name)) {
                std::wstring new_shader_name =
                    GenerateUniqueName(shader_name + L"_renamed_" + std::to_wstring(shader_id));
                library->DefineExport(new_shader_name.c_str(), shader_name.c_str());
                shader_name = new_shader_name;
            } else {
                library->DefineExport(shader_name.c_str());
            }
            shader_names_.insert(shader_name);
            shader_ids_[shader_id] = shader_name;
        }
    }

    size_t hit_group_count = 0;
    for (size_t i = 0; i < desc_.groups.size(); ++i) {
        if (desc_.groups[i].type == RayTracingShaderGroupType::kGeneral) {
            group_names_[i] = shader_ids_.at(desc_.groups[i].general);
            continue;
        }
        decltype(auto) hit_group = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
        std::wstring name = GenerateUniqueName(L"hit_group_" + std::to_wstring(hit_group_count++));
        hit_group->SetHitGroupExport(name.c_str());
        switch (desc_.groups[i].type) {
        case RayTracingShaderGroupType::kTrianglesHitGroup:
            if (desc_.groups[i].any_hit) {
                hit_group->SetAnyHitShaderImport(shader_ids_.at(desc_.groups[i].any_hit).c_str());
            }
            if (desc_.groups[i].closest_hit) {
                hit_group->SetClosestHitShaderImport(shader_ids_.at(desc_.groups[i].closest_hit).c_str());
            }
            break;
        case RayTracingShaderGroupType::kProceduralHitGroup:
            if (desc_.groups[i].intersection) {
                hit_group->SetIntersectionShaderImport(shader_ids_.at(desc_.groups[i].intersection).c_str());
            }
            break;
        default:
            break;
        }
        group_names_[i] = name;
    }

    decltype(auto) global_root_signature = subobjects.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    global_root_signature->SetRootSignature(root_signature_.Get());

    decltype(auto) shader_config = subobjects.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();

    uint32_t max_payload_size = 0;
    uint32_t max_attribute_size = 0;
    for (size_t i = 0; i < entry_points.size(); ++i) {
        max_payload_size = std::max(max_payload_size, entry_points[i].payload_size);
        max_attribute_size = std::max(max_attribute_size, entry_points[i].attribute_size);
    }
    shader_config->Config(max_payload_size, max_attribute_size);

    decltype(auto) pipeline_config = subobjects.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    pipeline_config->Config(1);

    ComPtr<ID3D12Device5> device5;
    device_.GetDevice().As(&device5);
    CHECK_HRESULT(device5->CreateStateObject(subobjects, IID_PPV_ARGS(&pipeline_state_)));
    pipeline_state_.As(&state_ojbect_props_);
}

std::wstring DXRayTracingPipeline::GenerateUniqueName(std::wstring name)
{
    static uint64_t id = 0;
    while (shader_names_.contains(name)) {
        name += L"_" + std::to_wstring(++id);
    }
    return name;
}

PipelineType DXRayTracingPipeline::GetPipelineType() const
{
    return PipelineType::kRayTracing;
}

const RayTracingPipelineDesc& DXRayTracingPipeline::GetDesc() const
{
    return desc_;
}

const ComPtr<ID3D12StateObject>& DXRayTracingPipeline::GetPipeline() const
{
    return pipeline_state_;
}

const ComPtr<ID3D12RootSignature>& DXRayTracingPipeline::GetRootSignature() const
{
    return root_signature_;
}

std::vector<uint8_t> DXRayTracingPipeline::GetRayTracingShaderGroupHandles(uint32_t first_group,
                                                                           uint32_t group_count) const
{
    std::vector<uint8_t> shader_handles_storage(group_count * device_.GetShaderGroupHandleSize());
    for (uint32_t i = 0; i < group_count; ++i) {
        memcpy(shader_handles_storage.data() + i * device_.GetShaderGroupHandleSize(),
               state_ojbect_props_->GetShaderIdentifier(group_names_.at(i + first_group).c_str()),
               device_.GetShaderGroupHandleSize());
    }
    return shader_handles_storage;
}

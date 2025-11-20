#pragma once
#include "Instance/BaseTypes.h"
#include "Pipeline/DXPipeline.h"

#if defined(_WIN32)
#include <wrl.h>
#else
#include <wsl/wrladapter.h>
#endif

#include <directx/d3d12.h>

#include <set>

using Microsoft::WRL::ComPtr;

class DXDevice;

class DXRayTracingPipeline : public DXPipeline {
public:
    DXRayTracingPipeline(DXDevice& device, const RayTracingPipelineDesc& desc);
    PipelineType GetPipelineType() const override;
    const ComPtr<ID3D12RootSignature>& GetRootSignature() const override;
    std::vector<uint8_t> GetRayTracingShaderGroupHandles(uint32_t first_group, uint32_t group_count) const override;

    const ComPtr<ID3D12StateObject>& GetPipeline() const;
    const RayTracingPipelineDesc& GetDesc() const;

private:
    std::wstring GenerateUniqueName(std::wstring name);

    DXDevice& device_;
    RayTracingPipelineDesc desc_;
    ComPtr<ID3D12RootSignature> root_signature_;
    ComPtr<ID3D12StateObject> pipeline_state_;
    std::map<uint64_t, std::wstring> shader_ids_;
    std::set<std::wstring> shader_names_;
    std::map<uint64_t, std::wstring> group_names_;
    ComPtr<ID3D12StateObjectProperties> state_ojbect_props_;
};

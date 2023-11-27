#pragma once
#include "Instance/BaseTypes.h"
#include "Pipeline/DXPipeline.h"

#include <directx/d3d12.h>
#include <wrl.h>

#include <set>
using namespace Microsoft::WRL;

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

    DXDevice& m_device;
    RayTracingPipelineDesc m_desc;
    ComPtr<ID3D12RootSignature> m_root_signature;
    ComPtr<ID3D12StateObject> m_pipeline_state;
    std::map<uint64_t, std::wstring> m_shader_ids;
    std::set<std::wstring> m_shader_names;
    std::map<uint64_t, std::wstring> m_group_names;
    ComPtr<ID3D12StateObjectProperties> m_state_ojbect_props;
};

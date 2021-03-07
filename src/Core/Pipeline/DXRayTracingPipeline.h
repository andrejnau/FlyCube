#pragma once
#include "Pipeline/DXPipeline.h"
#include <Instance/BaseTypes.h>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;

class DXRayTracingPipeline : public DXPipeline
{
public:
    DXRayTracingPipeline(DXDevice& device, const RayTracingPipelineDesc& desc);
    PipelineType GetPipelineType() const override;
    const ComPtr<ID3D12RootSignature>& GetRootSignature() const override;

    const ComPtr<ID3D12StateObject>& GetPipeline() const;
    const RayTracingPipelineDesc& GetDesc() const;
    const D3D12_DISPATCH_RAYS_DESC& GetDispatchRaysDesc() const;

private:
    void CreateShaderTable();

    DXDevice& m_device;
    RayTracingPipelineDesc m_desc;
    ComPtr<ID3D12RootSignature> m_root_signature;
    ComPtr<ID3D12StateObject> m_pipeline_state;
    D3D12_DISPATCH_RAYS_DESC m_raytrace_desc = {};
    ComPtr<ID3D12Resource> m_shader_table;
};

#pragma once
#include "Pipeline/Pipeline.h"
#include <Instance/BaseTypes.h>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;

class DXComputePipeline : public Pipeline
{
public:
    DXComputePipeline(DXDevice& device, const ComputePipelineDesc& desc);
    PipelineType GetPipelineType() const override;

    const ComputePipelineDesc& GetDesc() const;
    const ComPtr<ID3D12PipelineState>& GetPipeline() const;
    const ComPtr<ID3D12RootSignature>& GetRootSignature() const;

private:
    DXDevice& m_device;
    ComputePipelineDesc m_desc;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_input_layout_desc;
    std::map<size_t, std::string> m_input_layout_desc_names;
    D3D12_COMPUTE_PIPELINE_STATE_DESC m_compute_pso_desc = {};
    ComPtr<ID3D12RootSignature> m_root_signature;
    ComPtr<ID3D12PipelineState> m_pipeline_state;
};

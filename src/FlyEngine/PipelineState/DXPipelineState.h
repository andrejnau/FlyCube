#pragma once
#include "PipelineState/PipelineState.h"
#include <Instance/BaseTypes.h>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;

class DXPipelineState : public PipelineState
{
public:
    DXPipelineState(DXDevice& device, const PipelineStateDesc& desc);

private:
    void FillRTVFormats();
    void FillDSVFormat();
    std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout(const ComPtr<ID3DBlob>& blob);

    DXDevice& m_device;
    PipelineStateDesc m_desc;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_input_layout;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_graphics_pso_desc = {};
    D3D12_COMPUTE_PIPELINE_STATE_DESC m_compute_pso_desc = {};
    bool m_is_compute = false;
    ComPtr<ID3D12PipelineState> m_pipeline_state;
};

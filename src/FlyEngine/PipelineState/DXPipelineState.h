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

    const PipelineStateDesc& GetDesc() const;
    const ComPtr<ID3D12PipelineState>& GetPipelineState() const;
    const ComPtr<ID3D12RootSignature>& GetRootSignature() const;

private:
    void FillRTVFormats();
    void FillDSVFormat();
    void FillInputLayout(const ComPtr<ID3DBlob>& blob);

    DXDevice& m_device;
    PipelineStateDesc m_desc;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_input_layout_desc;
    std::map<size_t, std::string> m_input_layout_desc_names;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_graphics_pso_desc = {};
    D3D12_COMPUTE_PIPELINE_STATE_DESC m_compute_pso_desc = {};
    bool m_is_compute = false;
    ComPtr<ID3D12RootSignature> m_root_signature;
    ComPtr<ID3D12PipelineState> m_pipeline_state;
};

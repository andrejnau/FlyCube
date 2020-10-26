#pragma once
#include "Pipeline/Pipeline.h"
#include <Instance/BaseTypes.h>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;

class DXGraphicsPipeline : public Pipeline
{
public:
    DXGraphicsPipeline(DXDevice& device, const GraphicsPipelineDesc& desc);
    PipelineType GetPipelineType() const override;

    const GraphicsPipelineDesc& GetDesc() const;
    const ComPtr<ID3D12PipelineState>& GetPipeline() const;
    const ComPtr<ID3D12RootSignature>& GetRootSignature() const;
    const std::map<size_t, uint32_t>& GetStrideMap() const;

private:
    void ParseInputLayout();
    D3D12_INPUT_LAYOUT_DESC GetInputLayoutDesc();

    DXDevice& m_device;
    GraphicsPipelineDesc m_desc;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_input_layout_desc;
    std::map<size_t, std::string> m_input_layout_desc_names;
    std::map<size_t, uint32_t> m_input_layout_stride;
    ComPtr<ID3D12RootSignature> m_root_signature;
    ComPtr<ID3D12PipelineState> m_pipeline_state;
};

#pragma once
#include "Instance/BaseTypes.h"
#include "Pipeline/DXPipeline.h"

#if defined(_WIN32)
#include <wrl.h>
#else
#include <wsl/wrladapter.h>
#endif

#include <directx/d3d12.h>

#include <deque>

using Microsoft::WRL::ComPtr;

class DXDevice;
class Shader;

class DXGraphicsPipeline : public DXPipeline {
public:
    DXGraphicsPipeline(DXDevice& device, const GraphicsPipelineDesc& desc);
    PipelineType GetPipelineType() const override;
    const ComPtr<ID3D12RootSignature>& GetRootSignature() const override;

    const GraphicsPipelineDesc& GetDesc() const;
    const ComPtr<ID3D12PipelineState>& GetPipeline() const;
    const std::map<size_t, uint32_t>& GetStrideMap() const;

private:
    void ParseInputLayout(std::deque<std::string>& semantic_names);
    D3D12_INPUT_LAYOUT_DESC GetInputLayoutDesc();

    DXDevice& device_;
    GraphicsPipelineDesc desc_;
    std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout_desc_;
    std::map<size_t, uint32_t> input_layout_stride_;
    ComPtr<ID3D12RootSignature> root_signature_;
    ComPtr<ID3D12PipelineState> pipeline_state_;
};

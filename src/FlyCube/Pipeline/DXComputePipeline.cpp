#include "Pipeline/DXComputePipeline.h"

#include "BindingSetLayout/DXBindingSetLayout.h"
#include "Device/DXDevice.h"
#include "Pipeline/DXStateBuilder.h"
#include "Shader/Shader.h"
#include "Utilities/DXGIFormatHelper.h"
#include "View/DXView.h"

#include <directx/d3dx12.h>

#include <cassert>

DXComputePipeline::DXComputePipeline(DXDevice& device, const ComputePipelineDesc& desc)
    : device_(device)
    , desc_(desc)
{
    DXStateBuilder compute_state_builder;

    decltype(auto) dx_layout = desc_.layout->As<DXBindingSetLayout>();
    root_signature_ = dx_layout.GetRootSignature();

    decltype(auto) blob = desc_.shader->GetBlob();
    D3D12_SHADER_BYTECODE shader_bytecode = { blob.data(), blob.size() };
    assert(desc_.shader->GetType() == ShaderType::kCompute);
    compute_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_CS>(shader_bytecode);
    compute_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE>(root_signature_.Get());

    ComPtr<ID3D12Device2> device2;
    device_.GetDevice().As(&device2);
    auto pipeline_desc = compute_state_builder.GetDesc();
    CHECK_HRESULT(device2->CreatePipelineState(&pipeline_desc, IID_PPV_ARGS(&pipeline_state_)));
}

PipelineType DXComputePipeline::GetPipelineType() const
{
    return PipelineType::kCompute;
}

const ComputePipelineDesc& DXComputePipeline::GetDesc() const
{
    return desc_;
}

const ComPtr<ID3D12PipelineState>& DXComputePipeline::GetPipeline() const
{
    return pipeline_state_;
}

const ComPtr<ID3D12RootSignature>& DXComputePipeline::GetRootSignature() const
{
    return root_signature_;
}

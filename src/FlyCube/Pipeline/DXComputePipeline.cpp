#include "Pipeline/DXComputePipeline.h"

#include "BindingSetLayout/DXBindingSetLayout.h"
#include "Device/DXDevice.h"
#include "Pipeline/DXStateBuilder.h"
#include "Program/ProgramBase.h"
#include "Shader/Shader.h"
#include "Utilities/DXGIFormatHelper.h"
#include "View/DXView.h"

#include <directx/d3dx12.h>

DXComputePipeline::DXComputePipeline(DXDevice& device, const ComputePipelineDesc& desc)
    : m_device(device)
    , m_desc(desc)
{
    DXStateBuilder compute_state_builder;

    decltype(auto) dx_program = m_desc.program->As<ProgramBase>();
    decltype(auto) dx_layout = m_desc.layout->As<DXBindingSetLayout>();
    m_root_signature = dx_layout.GetRootSignature();
    for (const auto& shader : dx_program.GetShaders()) {
        D3D12_SHADER_BYTECODE ShaderBytecode = {};
        decltype(auto) blob = shader->GetBlob();
        ShaderBytecode.pShaderBytecode = blob.data();
        ShaderBytecode.BytecodeLength = blob.size();
        switch (shader->GetType()) {
        case ShaderType::kCompute: {
            compute_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_CS>(ShaderBytecode);
            break;
        }
        }
    }
    compute_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE>(m_root_signature.Get());

    ComPtr<ID3D12Device2> device2;
    m_device.GetDevice().As(&device2);
    ASSERT_SUCCEEDED(device2->CreatePipelineState(&compute_state_builder.GetDesc(), IID_PPV_ARGS(&m_pipeline_state)));
}

PipelineType DXComputePipeline::GetPipelineType() const
{
    return PipelineType::kCompute;
}

const ComputePipelineDesc& DXComputePipeline::GetDesc() const
{
    return m_desc;
}

const ComPtr<ID3D12PipelineState>& DXComputePipeline::GetPipeline() const
{
    return m_pipeline_state;
}

const ComPtr<ID3D12RootSignature>& DXComputePipeline::GetRootSignature() const
{
    return m_root_signature;
}

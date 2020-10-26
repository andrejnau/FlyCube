#include "Pipeline/DXComputePipeline.h"
#include <Device/DXDevice.h>
#include <Program/DXProgram.h>
#include <Shader/DXShader.h>
#include <HLSLCompiler/DXReflector.h>
#include <View/DXView.h>
#include <Utilities/DXGIFormatHelper.h>
#include <d3dx12.h>

struct ComputePipelineStateStream
{
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
    CD3DX12_PIPELINE_STATE_STREAM_CS CS;
};

DXComputePipeline::DXComputePipeline(DXDevice& device, const ComputePipelineDesc& desc)
    : m_device(device)
    , m_desc(desc)
{
    ComputePipelineStateStream compute_stream_desc = {};
    decltype(auto) dx_program = m_desc.program->As<DXProgram>();
    m_root_signature = dx_program.GetRootSignature();
    for (const auto& shader : dx_program.GetShaders())
    {
        D3D12_SHADER_BYTECODE ShaderBytecode = {};
        auto blob = shader->GetBlob();
        ShaderBytecode.BytecodeLength = blob->GetBufferSize();
        ShaderBytecode.pShaderBytecode = blob->GetBufferPointer();
        switch (shader->GetType())
        {
        case ShaderType::kCompute:
        {
            compute_stream_desc.CS = ShaderBytecode;
            break;
        }
        }
    }
    compute_stream_desc.pRootSignature = m_root_signature.Get();

    D3D12_PIPELINE_STATE_STREAM_DESC stream_desc = {};
    stream_desc.pPipelineStateSubobjectStream = &compute_stream_desc;
    stream_desc.SizeInBytes = sizeof(compute_stream_desc);

    ComPtr<ID3D12Device2> device2;
    m_device.GetDevice().As(&device2);
    ASSERT_SUCCEEDED(device2->CreatePipelineState(&stream_desc, IID_PPV_ARGS(&m_pipeline_state)));
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

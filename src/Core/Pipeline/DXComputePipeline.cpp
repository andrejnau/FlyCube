#include "Pipeline/DXComputePipeline.h"
#include <Device/DXDevice.h>
#include <Program/DXProgram.h>
#include <Shader/DXShader.h>
#include <Shader/DXReflector.h>
#include <View/DXView.h>
#include <Utilities/DXGIFormatHelper.h>
#include <d3dx12.h>

DXComputePipeline::DXComputePipeline(DXDevice& device, const ComputePipelineDesc& desc)
    : m_device(device)
    , m_desc(desc)
{
    decltype(auto) dx_program = m_desc.program->As<DXProgram>();
    auto shaders = dx_program.GetShaders();
    for (auto& shader : shaders)
    {
        D3D12_SHADER_BYTECODE ShaderBytecode = {};
        auto blob = shader->GetBlob();
        ShaderBytecode.BytecodeLength = blob->GetBufferSize();
        ShaderBytecode.pShaderBytecode = blob->GetBufferPointer();
        switch (shader->GetType())
        {
        case ShaderType::kCompute:
        {
            m_compute_pso_desc.CS = ShaderBytecode;
            break;
        }
        }
    }

    m_root_signature = dx_program.GetRootSignature();
    m_compute_pso_desc.pRootSignature = m_root_signature.Get();
    ASSERT_SUCCEEDED(m_device.GetDevice()->CreateComputePipelineState(&m_compute_pso_desc, IID_PPV_ARGS(&m_pipeline_state)));
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

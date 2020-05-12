#include "PipelineState/DXPipelineState.h"
#include <Device/DXDevice.h>
#include <PipelineProgram/DXPipelineProgram.h>
#include <Shader/DXShader.h>
#include <Shader/DXReflector.h>
#include <View/DXView.h>
#include <Utilities/DXGIFormatHelper.h>
#include <d3dx12.h>

DXPipelineState::DXPipelineState(DXDevice& device, const PipelineStateDesc& desc)
    : m_device(device)
    , m_desc(desc)
{
    m_graphics_pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    m_graphics_pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    CD3DX12_DEPTH_STENCIL_DESC depth_stencil_desc(D3D12_DEFAULT);
    depth_stencil_desc.DepthEnable = false;
    depth_stencil_desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depth_stencil_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depth_stencil_desc.StencilEnable = FALSE;
    m_graphics_pso_desc.DepthStencilState = depth_stencil_desc;
    m_graphics_pso_desc.SampleMask = UINT_MAX;
    m_graphics_pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    m_graphics_pso_desc.SampleDesc.Count = 1;

    DXPipelineProgram& dx_program = static_cast<DXPipelineProgram&>(*m_desc.program);
    auto shaders = dx_program.GetShaders();
    for (auto& shader : shaders)
    {
        D3D12_SHADER_BYTECODE ShaderBytecode = {};
        auto blob = shader->GetBlob();
        ShaderBytecode.BytecodeLength = blob->GetBufferSize();
        ShaderBytecode.pShaderBytecode = blob->GetBufferPointer();
        switch (shader->GetType())
        {
        case ShaderType::kVertex:
        {
            m_graphics_pso_desc.VS = ShaderBytecode;
            m_input_layout = GetInputLayout(blob);
            m_graphics_pso_desc.InputLayout.NumElements = static_cast<uint32_t>(m_input_layout.size());
            m_graphics_pso_desc.InputLayout.pInputElementDescs = m_input_layout.data();
            break;
        }
        case ShaderType::kPixel:
            m_graphics_pso_desc.PS = ShaderBytecode;
            FillRTVFormats();
            FillDSVFormat();
            break;
        case ShaderType::kGeometry:
            m_graphics_pso_desc.GS = ShaderBytecode;
            break;
        case ShaderType::kCompute:
            m_compute_pso_desc.CS = ShaderBytecode;
            m_is_compute = true;
            break;
        }
    }

    if (m_is_compute)
    {
        m_compute_pso_desc.pRootSignature = dx_program.GetRootSignature().Get();
        ASSERT_SUCCEEDED(m_device.GetDevice()->CreateComputePipelineState(&m_compute_pso_desc, IID_PPV_ARGS(&m_pipeline_state)));
    }
    else
    {
        m_graphics_pso_desc.pRootSignature = dx_program.GetRootSignature().Get();
        ASSERT_SUCCEEDED(m_device.GetDevice()->CreateGraphicsPipelineState(&m_graphics_pso_desc, IID_PPV_ARGS(&m_pipeline_state)));
    }
}

void DXPipelineState::FillRTVFormats()
{
    for (size_t slot = 0; slot < m_desc.rtvs.size(); ++slot)
    {
        auto& dx_view = static_cast<DXView&>(*m_desc.rtvs[slot]);
        m_graphics_pso_desc.RTVFormats[slot] = dx_view.GetFormat();
    }
}

void DXPipelineState::FillDSVFormat()
{
    auto& dx_view = static_cast<DXView&>(*m_desc.dsv);
    m_graphics_pso_desc.DSVFormat = dx_view.GetFormat();
    m_graphics_pso_desc.DepthStencilState.DepthEnable = m_graphics_pso_desc.DSVFormat != DXGI_FORMAT_UNKNOWN;
}

std::vector<D3D12_INPUT_ELEMENT_DESC> DXPipelineState::GetInputLayout(const ComPtr<ID3DBlob>& blob)
{
    ComPtr<ID3D12ShaderReflection> reflector;
    DXReflect(blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&reflector));
    D3D12_SHADER_DESC shader_desc = {};
    reflector->GetDesc(&shader_desc);

    std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout_desc;
    for (uint32_t i = 0; i < shader_desc.InputParameters; ++i)
    {
        D3D12_SIGNATURE_PARAMETER_DESC param_desc = {};
        reflector->GetInputParameterDesc(i, &param_desc);

        D3D12_INPUT_ELEMENT_DESC layout = {};
        layout.SemanticName = param_desc.SemanticName;
        layout.SemanticIndex = param_desc.SemanticIndex;
        layout.InputSlot = i;
        layout.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        layout.InstanceDataStepRate = 0;

        if (param_desc.Mask == 1)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.Format = DXGI_FORMAT_R32_UINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.Format = DXGI_FORMAT_R32_SINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.Format = DXGI_FORMAT_R32_FLOAT;
        }
        else if (param_desc.Mask <= 3)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.Format = DXGI_FORMAT_R32G32_UINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.Format = DXGI_FORMAT_R32G32_SINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.Format = DXGI_FORMAT_R32G32_FLOAT;
        }
        else if (param_desc.Mask <= 7)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if (param_desc.Mask <= 15)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
        input_layout_desc.push_back(layout);
    }

    return input_layout_desc;
}
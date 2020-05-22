#include "Pipeline/DXGraphicsPipeline.h"
#include <Device/DXDevice.h>
#include <Program/DXProgram.h>
#include <Shader/DXShader.h>
#include <Shader/DXReflector.h>
#include <View/DXView.h>
#include <Utilities/DXGIFormatHelper.h>
#include <d3dx12.h>

DXGraphicsPipeline::DXGraphicsPipeline(DXDevice& device, const GraphicsPipelineDesc& desc)
    : m_device(device)
    , m_desc(desc)
{
    m_graphics_pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    m_graphics_pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    CD3DX12_DEPTH_STENCIL_DESC depth_stencil_desc(D3D12_DEFAULT);
    depth_stencil_desc.DepthEnable = desc.depth_desc.depth_enable;
    depth_stencil_desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depth_stencil_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depth_stencil_desc.StencilEnable = false;

    if (desc.depth_desc.func == DepthComparison::kLess)
        depth_stencil_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    else if (desc.depth_desc.func == DepthComparison::kLessEqual)
        depth_stencil_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    m_graphics_pso_desc.DepthStencilState = depth_stencil_desc;
    m_graphics_pso_desc.SampleMask = std::numeric_limits<uint32_t>::max();
    m_graphics_pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    m_graphics_pso_desc.SampleDesc.Count = 1;
    m_graphics_pso_desc.NumRenderTargets = 0;

    {
        switch (desc.rasterizer_desc.fill_mode)
        {
        case FillMode::kWireframe:
            m_graphics_pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
            break;
        case FillMode::kSolid:
            m_graphics_pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            break;
        }

        switch (desc.rasterizer_desc.cull_mode)
        {
        case CullMode::kNone:
            m_graphics_pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
            break;
        case CullMode::kFront:
            m_graphics_pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
            break;
        case CullMode::kBack:
            m_graphics_pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
            break;
        }

        m_graphics_pso_desc.RasterizerState.DepthBias = desc.rasterizer_desc.DepthBias;
    }


    auto convert = [](Blend type)
    {
        switch (type)
        {
        case Blend::kZero:
            return D3D12_BLEND_ZERO;
        case Blend::kSrcAlpha:
            return D3D12_BLEND_SRC_ALPHA;
        case Blend::kInvSrcAlpha:
            return D3D12_BLEND_INV_SRC_ALPHA;
        }
        return static_cast<D3D12_BLEND>(0);
    };

    auto convert_op = [](BlendOp type)
    {
        switch (type)
        {
        case BlendOp::kAdd:
            return D3D12_BLEND_OP_ADD;
        }
        return static_cast<D3D12_BLEND_OP>(0);
    };

    for (auto& rtv : m_desc.rtvs)
    {
        if (rtv.format == gli::format::FORMAT_UNDEFINED)
            continue;
        decltype(auto) rt_desc = m_graphics_pso_desc.BlendState.RenderTarget[rtv.slot];
        rt_desc.BlendEnable = desc.blend_desc.blend_enable;
        rt_desc.BlendOp = convert_op(desc.blend_desc.blend_op);
        rt_desc.SrcBlend = convert(desc.blend_desc.blend_src);
        rt_desc.DestBlend = convert(desc.blend_desc.blend_dest);
        rt_desc.BlendOpAlpha = convert_op(desc.blend_desc.blend_op_alpha);
        rt_desc.SrcBlendAlpha = convert(desc.blend_desc.blend_src_alpha);
        rt_desc.DestBlendAlpha = convert(desc.blend_desc.blend_dest_apha);
        rt_desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }

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
        case ShaderType::kVertex:
        {
            m_graphics_pso_desc.VS = ShaderBytecode;
            FillInputLayout();
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
        }
    }
    m_root_signature = dx_program.GetRootSignature();
    m_graphics_pso_desc.pRootSignature = m_root_signature.Get();
    ASSERT_SUCCEEDED(m_device.GetDevice()->CreateGraphicsPipelineState(&m_graphics_pso_desc, IID_PPV_ARGS(&m_pipeline_state)));
}

PipelineType DXGraphicsPipeline::GetPipelineType() const
{
    return PipelineType::kGraphics;
}

void DXGraphicsPipeline::FillRTVFormats()
{
    for (auto& rtv : m_desc.rtvs)
    {
        if (rtv.format == gli::format::FORMAT_UNDEFINED)
            continue;
        m_graphics_pso_desc.NumRenderTargets = std::max(m_graphics_pso_desc.NumRenderTargets, rtv.slot + 1);
        m_graphics_pso_desc.RTVFormats[rtv.slot] = static_cast<DXGI_FORMAT>(gli::dx().translate(rtv.format).DXGIFormat.DDS);
    }
}

void DXGraphicsPipeline::FillDSVFormat()
{
    if (m_desc.dsv.format == gli::format::FORMAT_UNDEFINED)
        return;
    m_graphics_pso_desc.DSVFormat = static_cast<DXGI_FORMAT>(gli::dx().translate(m_desc.dsv.format).DXGIFormat.DDS);
    m_graphics_pso_desc.DepthStencilState.DepthEnable = m_graphics_pso_desc.DSVFormat != DXGI_FORMAT_UNKNOWN;
}

void DXGraphicsPipeline::FillInputLayout()
{
    for (auto& vertex : m_desc.input)
    {
        D3D12_INPUT_ELEMENT_DESC layout = {};

        std::string semantic_name = vertex.semantic_name;
        uint32_t semantic_slot = 0;
        uint32_t pow = 1;
        while (!semantic_name.empty() && std::isdigit(semantic_name.back()))
        {
            semantic_slot = (semantic_name.back() - '0') * pow + semantic_slot;
            semantic_name.pop_back();
            pow *= 10;
        }
        m_input_layout_desc_names[vertex.slot] = semantic_name;
        layout.SemanticName = m_input_layout_desc_names[vertex.slot].c_str();
        layout.SemanticIndex = semantic_slot;
        layout.InputSlot = vertex.slot;
        layout.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        layout.InstanceDataStepRate = 0;
        layout.Format = static_cast<DXGI_FORMAT>(gli::dx().translate(vertex.format).DXGIFormat.DDS);
        m_input_layout_desc.push_back(layout);
    }

    m_graphics_pso_desc.InputLayout.NumElements = static_cast<uint32_t>(m_input_layout_desc.size());
    m_graphics_pso_desc.InputLayout.pInputElementDescs = m_input_layout_desc.data();
}

const GraphicsPipelineDesc& DXGraphicsPipeline::GetDesc() const
{
    return m_desc;
}

const ComPtr<ID3D12PipelineState>& DXGraphicsPipeline::GetPipeline() const
{
    return m_pipeline_state;
}

const ComPtr<ID3D12RootSignature>& DXGraphicsPipeline::GetRootSignature() const
{
    return m_root_signature;
}

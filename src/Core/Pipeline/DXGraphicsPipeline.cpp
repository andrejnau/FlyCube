#include "Pipeline/DXGraphicsPipeline.h"
#include <Device/DXDevice.h>
#include <Program/DXProgram.h>
#include <Shader/DXShader.h>
#include <HLSLCompiler/DXReflector.h>
#include <View/DXView.h>
#include <Utilities/DXGIFormatHelper.h>
#include <d3dx12.h>

struct GraphicsPipelineStateStream
{
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
    CD3DX12_PIPELINE_STATE_STREAM_VS VS;
    CD3DX12_PIPELINE_STATE_STREAM_GS GS;
    CD3DX12_PIPELINE_STATE_STREAM_PS PS;
    CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BlendState;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencilState;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
    CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RasterizerState;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
};

CD3DX12_RASTERIZER_DESC GetRasterizerDesc(const GraphicsPipelineDesc& desc)
{
    CD3DX12_RASTERIZER_DESC rasterizer_desc = {};
    switch (desc.rasterizer_desc.fill_mode)
    {
    case FillMode::kWireframe:
        rasterizer_desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
        break;
    case FillMode::kSolid:
        rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
        break;
    }
    switch (desc.rasterizer_desc.cull_mode)
    {
    case CullMode::kNone:
        rasterizer_desc.CullMode = D3D12_CULL_MODE_NONE;
        break;
    case CullMode::kFront:
        rasterizer_desc.CullMode = D3D12_CULL_MODE_FRONT;
        break;
    case CullMode::kBack:
        rasterizer_desc.CullMode = D3D12_CULL_MODE_BACK;
        break;
    }
    rasterizer_desc.DepthBias = desc.rasterizer_desc.depth_bias;
    return rasterizer_desc;
}

CD3DX12_BLEND_DESC GetBlendDesc(const GraphicsPipelineDesc& desc)
{
    CD3DX12_BLEND_DESC blend_desc = {};
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
    const RenderPassDesc& render_pass_desc = desc.render_pass->GetDesc();
    for (size_t i = 0; i < render_pass_desc.colors.size(); ++i)
    {
        if (render_pass_desc.colors[i].format == gli::format::FORMAT_UNDEFINED)
            continue;
        decltype(auto) rt_desc = blend_desc.RenderTarget[i];
        rt_desc.BlendEnable = desc.blend_desc.blend_enable;
        rt_desc.BlendOp = convert_op(desc.blend_desc.blend_op);
        rt_desc.SrcBlend = convert(desc.blend_desc.blend_src);
        rt_desc.DestBlend = convert(desc.blend_desc.blend_dest);
        rt_desc.BlendOpAlpha = convert_op(desc.blend_desc.blend_op_alpha);
        rt_desc.SrcBlendAlpha = convert(desc.blend_desc.blend_src_alpha);
        rt_desc.DestBlendAlpha = convert(desc.blend_desc.blend_dest_apha);
        rt_desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }
    return blend_desc;
}

CD3DX12_DEPTH_STENCIL_DESC GetDepthStencilDesc(const GraphicsPipelineDesc& desc, DXGI_FORMAT dsv_format)
{
    CD3DX12_DEPTH_STENCIL_DESC depth_stencil_desc(D3D12_DEFAULT);
    depth_stencil_desc.DepthEnable = desc.depth_desc.depth_enable;
    depth_stencil_desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depth_stencil_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depth_stencil_desc.StencilEnable = false;

    if (desc.depth_desc.func == DepthComparison::kLess)
        depth_stencil_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    else if (desc.depth_desc.func == DepthComparison::kLessEqual)
        depth_stencil_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    if (dsv_format == DXGI_FORMAT_UNKNOWN)
        depth_stencil_desc.DepthEnable = false;

    return depth_stencil_desc;
}

D3D12_RT_FORMAT_ARRAY GetRTVFormats(const GraphicsPipelineDesc& desc)
{
    const RenderPassDesc& render_pass_desc = desc.render_pass->GetDesc();
    D3D12_RT_FORMAT_ARRAY rt_formats = {};
    for (size_t i = 0; i < render_pass_desc.colors.size(); ++i)
    {
        if (render_pass_desc.colors[i].format == gli::format::FORMAT_UNDEFINED)
            continue;
        rt_formats.NumRenderTargets = i + 1;
        rt_formats.RTFormats[i] = static_cast<DXGI_FORMAT>(gli::dx().translate(render_pass_desc.colors[i].format).DXGIFormat.DDS);
    }
    return rt_formats;
}

DXGI_FORMAT GetDSVFormat(const GraphicsPipelineDesc& desc)
{
    const RenderPassDesc& render_pass_desc = desc.render_pass->GetDesc();
    if (render_pass_desc.depth_stencil.format == gli::format::FORMAT_UNDEFINED)
        return DXGI_FORMAT_UNKNOWN;
    return static_cast<DXGI_FORMAT>(gli::dx().translate(render_pass_desc.depth_stencil.format).DXGIFormat.DDS);
}

DXGI_SAMPLE_DESC GetSampleDesc(const GraphicsPipelineDesc& desc)
{
    const RenderPassDesc& render_pass_desc = desc.render_pass->GetDesc();
    return { render_pass_desc.sample_count, 0 };
}

DXGraphicsPipeline::DXGraphicsPipeline(DXDevice& device, const GraphicsPipelineDesc& desc)
    : m_device(device)
    , m_desc(desc)
{
    GraphicsPipelineStateStream graphics_stream_desc = {};

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
        case ShaderType::kVertex:
        {
            graphics_stream_desc.VS = ShaderBytecode;
            ParseInputLayout();
            graphics_stream_desc.InputLayout = GetInputLayoutDesc();
            break;
        }
        case ShaderType::kGeometry:
            graphics_stream_desc.GS = ShaderBytecode;
            break;
        case ShaderType::kPixel:
            graphics_stream_desc.PS = ShaderBytecode;
            graphics_stream_desc.RTVFormats = GetRTVFormats(desc);
            graphics_stream_desc.DSVFormat = GetDSVFormat(desc);
            break;
        }
    }

    graphics_stream_desc.DepthStencilState = GetDepthStencilDesc(desc, graphics_stream_desc.DSVFormat);
    graphics_stream_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    graphics_stream_desc.SampleDesc = GetSampleDesc(desc);
    graphics_stream_desc.RasterizerState = GetRasterizerDesc(desc);
    graphics_stream_desc.BlendState = GetBlendDesc(desc);
    graphics_stream_desc.pRootSignature = m_root_signature.Get();

    D3D12_PIPELINE_STATE_STREAM_DESC stream_desc = {};
    stream_desc.pPipelineStateSubobjectStream = &graphics_stream_desc;
    stream_desc.SizeInBytes = sizeof(graphics_stream_desc);

    ComPtr<ID3D12Device2> device2;
    m_device.GetDevice().As(&device2);
    ASSERT_SUCCEEDED(device2->CreatePipelineState(&stream_desc, IID_PPV_ARGS(&m_pipeline_state)));
}

void DXGraphicsPipeline::ParseInputLayout()
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
        m_input_layout_stride[vertex.slot] = vertex.stride;
        m_input_layout_desc_names[vertex.slot] = semantic_name;
        layout.SemanticName = m_input_layout_desc_names[vertex.slot].c_str();
        layout.SemanticIndex = semantic_slot;
        layout.InputSlot = vertex.slot;
        layout.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        layout.InstanceDataStepRate = 0;
        layout.Format = static_cast<DXGI_FORMAT>(gli::dx().translate(vertex.format).DXGIFormat.DDS);
        m_input_layout_desc.push_back(layout);
    }
}

D3D12_INPUT_LAYOUT_DESC DXGraphicsPipeline::GetInputLayoutDesc()
{
    D3D12_INPUT_LAYOUT_DESC input_layout_desc = {};
    input_layout_desc.NumElements = static_cast<uint32_t>(m_input_layout_desc.size());
    input_layout_desc.pInputElementDescs = m_input_layout_desc.data();
    return input_layout_desc;
}

PipelineType DXGraphicsPipeline::GetPipelineType() const
{
    return PipelineType::kGraphics;
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

const std::map<size_t, uint32_t>& DXGraphicsPipeline::GetStrideMap() const
{
    return m_input_layout_stride;
}

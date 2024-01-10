#include "Pipeline/DXGraphicsPipeline.h"

#include "BindingSetLayout/DXBindingSetLayout.h"
#include "Device/DXDevice.h"
#include "Pipeline/DXStateBuilder.h"
#include "Program/ProgramBase.h"
#include "View/DXView.h"

#include <directx/d3dx12.h>
#include <gli/gli.hpp>

#include <cctype>

CD3DX12_RASTERIZER_DESC GetRasterizerDesc(const GraphicsPipelineDesc& desc)
{
    CD3DX12_RASTERIZER_DESC rasterizer_desc(D3D12_DEFAULT);
    switch (desc.rasterizer_desc.fill_mode) {
    case FillMode::kWireframe:
        rasterizer_desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
        break;
    case FillMode::kSolid:
        rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
        break;
    }
    switch (desc.rasterizer_desc.cull_mode) {
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
    CD3DX12_BLEND_DESC blend_desc(D3D12_DEFAULT);
    auto convert = [](Blend type) {
        switch (type) {
        case Blend::kZero:
            return D3D12_BLEND_ZERO;
        case Blend::kSrcAlpha:
            return D3D12_BLEND_SRC_ALPHA;
        case Blend::kInvSrcAlpha:
            return D3D12_BLEND_INV_SRC_ALPHA;
        }
        return static_cast<D3D12_BLEND>(0);
    };
    auto convert_op = [](BlendOp type) {
        switch (type) {
        case BlendOp::kAdd:
            return D3D12_BLEND_OP_ADD;
        }
        return static_cast<D3D12_BLEND_OP>(0);
    };
    const RenderPassDesc& render_pass_desc = desc.render_pass->GetDesc();
    for (size_t i = 0; i < render_pass_desc.colors.size(); ++i) {
        if (render_pass_desc.colors[i].format == gli::format::FORMAT_UNDEFINED) {
            continue;
        }
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

D3D12_COMPARISON_FUNC Convert(ComparisonFunc func)
{
    switch (func) {
    case ComparisonFunc::kNever:
        return D3D12_COMPARISON_FUNC_NEVER;
    case ComparisonFunc::kLess:
        return D3D12_COMPARISON_FUNC_LESS;
    case ComparisonFunc::kEqual:
        return D3D12_COMPARISON_FUNC_EQUAL;
    case ComparisonFunc::kLessEqual:
        return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case ComparisonFunc::kGreater:
        return D3D12_COMPARISON_FUNC_GREATER;
    case ComparisonFunc::kNotEqual:
        return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case ComparisonFunc::kGreaterEqual:
        return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case ComparisonFunc::kAlways:
        return D3D12_COMPARISON_FUNC_ALWAYS;
    default:
        assert(false);
        return D3D12_COMPARISON_FUNC_LESS;
    }
}

D3D12_STENCIL_OP Convert(StencilOp op)
{
    switch (op) {
    case StencilOp::kKeep:
        return D3D12_STENCIL_OP_KEEP;
    case StencilOp::kZero:
        return D3D12_STENCIL_OP_ZERO;
    case StencilOp::kReplace:
        return D3D12_STENCIL_OP_REPLACE;
    case StencilOp::kIncrSat:
        return D3D12_STENCIL_OP_INCR_SAT;
    case StencilOp::kDecrSat:
        return D3D12_STENCIL_OP_DECR_SAT;
    case StencilOp::kInvert:
        return D3D12_STENCIL_OP_INVERT;
    case StencilOp::kIncr:
        return D3D12_STENCIL_OP_INCR;
    case StencilOp::kDecr:
        return D3D12_STENCIL_OP_DECR;
    default:
        assert(false);
        return D3D12_STENCIL_OP_KEEP;
    }
}

D3D12_DEPTH_STENCILOP_DESC Convert(const StencilOpDesc& desc)
{
    D3D12_DEPTH_STENCILOP_DESC res = {};
    res.StencilFailOp = Convert(desc.fail_op);
    res.StencilPassOp = Convert(desc.pass_op);
    res.StencilDepthFailOp = Convert(desc.depth_fail_op);
    res.StencilFunc = Convert(desc.func);
    return res;
}

CD3DX12_DEPTH_STENCIL_DESC1 GetDepthStencilDesc(const DepthStencilDesc& desc, DXGI_FORMAT dsv_format)
{
    CD3DX12_DEPTH_STENCIL_DESC1 depth_stencil_desc(D3D12_DEFAULT);
    depth_stencil_desc.DepthEnable = desc.depth_test_enable;
    depth_stencil_desc.DepthWriteMask =
        desc.depth_write_enable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    depth_stencil_desc.DepthBoundsTestEnable = desc.depth_bounds_test_enable;
    depth_stencil_desc.DepthFunc = Convert(desc.depth_func);
    depth_stencil_desc.StencilEnable = desc.stencil_enable;
    depth_stencil_desc.StencilReadMask = desc.stencil_read_mask;
    depth_stencil_desc.StencilWriteMask = desc.stencil_write_mask;
    depth_stencil_desc.FrontFace = Convert(desc.front_face);
    depth_stencil_desc.BackFace = Convert(desc.back_face);

    if (dsv_format == DXGI_FORMAT_UNKNOWN) {
        depth_stencil_desc.DepthEnable = false;
    }

    return depth_stencil_desc;
}

D3D12_RT_FORMAT_ARRAY GetRTVFormats(const GraphicsPipelineDesc& desc)
{
    const RenderPassDesc& render_pass_desc = desc.render_pass->GetDesc();
    D3D12_RT_FORMAT_ARRAY rt_formats = {};
    for (size_t i = 0; i < render_pass_desc.colors.size(); ++i) {
        if (render_pass_desc.colors[i].format == gli::format::FORMAT_UNDEFINED) {
            continue;
        }
        rt_formats.NumRenderTargets = i + 1;
        rt_formats.RTFormats[i] =
            static_cast<DXGI_FORMAT>(gli::dx().translate(render_pass_desc.colors[i].format).DXGIFormat.DDS);
    }
    return rt_formats;
}

DXGI_FORMAT GetDSVFormat(const GraphicsPipelineDesc& desc)
{
    const RenderPassDesc& render_pass_desc = desc.render_pass->GetDesc();
    if (render_pass_desc.depth_stencil.format == gli::format::FORMAT_UNDEFINED) {
        return DXGI_FORMAT_UNKNOWN;
    }
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
    DXStateBuilder graphics_state_builder;

    decltype(auto) dx_program = m_desc.program->As<ProgramBase>();
    decltype(auto) dx_layout = m_desc.layout->As<DXBindingSetLayout>();
    m_root_signature = dx_layout.GetRootSignature();
    for (const auto& shader : dx_program.GetShaders()) {
        D3D12_SHADER_BYTECODE ShaderBytecode = {};
        decltype(auto) blob = shader->GetBlob();
        ShaderBytecode.pShaderBytecode = blob.data();
        ShaderBytecode.BytecodeLength = blob.size();

        switch (shader->GetType()) {
        case ShaderType::kVertex: {
            graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_VS>(ShaderBytecode);
            ParseInputLayout(shader);
            graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT>(GetInputLayoutDesc());
            graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT>(GetDSVFormat(desc));
            break;
        }
        case ShaderType::kGeometry:
            graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_GS>(ShaderBytecode);
            break;
        case ShaderType::kAmplification:
            graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_AS>(ShaderBytecode);
            break;
        case ShaderType::kMesh:
            graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_MS>(ShaderBytecode);
            graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT>(GetDSVFormat(desc));
            break;
        case ShaderType::kPixel:
            graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_PS>(ShaderBytecode);
            graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS>(GetRTVFormats(desc));
            break;
        }
    }

    graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1>(
        GetDepthStencilDesc(desc.depth_stencil_desc, GetDSVFormat(desc)));
    graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC>(GetSampleDesc(desc));
    graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER>(GetRasterizerDesc(desc));
    graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC>(GetBlendDesc(desc));
    graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE>(m_root_signature.Get());

    ComPtr<ID3D12Device2> device2;
    m_device.GetDevice().As(&device2);
    ASSERT_SUCCEEDED(device2->CreatePipelineState(&graphics_state_builder.GetDesc(), IID_PPV_ARGS(&m_pipeline_state)));
}

void DXGraphicsPipeline::ParseInputLayout(const std::shared_ptr<Shader>& shader)
{
    for (auto& vertex : m_desc.input) {
        D3D12_INPUT_ELEMENT_DESC layout = {};
        std::string semantic_name = vertex.semantic_name;
        uint32_t semantic_slot = 0;
        uint32_t pow = 1;
        while (!semantic_name.empty() && std::isdigit(semantic_name.back())) {
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

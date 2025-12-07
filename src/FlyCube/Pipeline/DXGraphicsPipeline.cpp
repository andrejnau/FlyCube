#include "Pipeline/DXGraphicsPipeline.h"

#include "BindingSetLayout/DXBindingSetLayout.h"
#include "Device/DXDevice.h"
#include "Pipeline/DXStateBuilder.h"
#include "Utilities/Check.h"
#include "Utilities/NotReached.h"
#include "View/DXView.h"

#include <directx/d3dx12.h>
#include <gli/gli.hpp>

#include <cctype>

namespace {

D3D12_FILL_MODE ConvertFillMode(FillMode fill_mode)
{
    switch (fill_mode) {
    case FillMode::kSolid:
        return D3D12_FILL_MODE_SOLID;
    case FillMode::kWireframe:
        return D3D12_FILL_MODE_WIREFRAME;
    default:
        NOTREACHED();
    }
}

D3D12_CULL_MODE ConvertCullMode(CullMode cull_mode)
{
    switch (cull_mode) {
    case CullMode::kNone:
        return D3D12_CULL_MODE_NONE;
    case CullMode::kFront:
        return D3D12_CULL_MODE_FRONT;
    case CullMode::kBack:
        return D3D12_CULL_MODE_BACK;
    default:
        NOTREACHED();
    }
}

CD3DX12_RASTERIZER_DESC GetRasterizerDesc(const RasterizerDesc& desc)
{
    CD3DX12_RASTERIZER_DESC rasterizer_desc(D3D12_DEFAULT);
    rasterizer_desc.FillMode = ConvertFillMode(desc.fill_mode);
    rasterizer_desc.CullMode = ConvertCullMode(desc.cull_mode);
    rasterizer_desc.FrontCounterClockwise = desc.front_face == FrontFace::kCounterClockwise;
    rasterizer_desc.DepthBias = desc.depth_bias;
    rasterizer_desc.DepthBiasClamp = desc.depth_bias_clamp;
    rasterizer_desc.SlopeScaledDepthBias = desc.slope_scaled_depth_bias;
    rasterizer_desc.DepthClipEnable = desc.depth_bias_clamp;
    return rasterizer_desc;
}

D3D12_BLEND_OP ConvertBlendOp(BlendOp op)
{
    switch (op) {
    case BlendOp::kAdd:
        return D3D12_BLEND_OP_ADD;
    case BlendOp::kSubtract:
        return D3D12_BLEND_OP_SUBTRACT;
    case BlendOp::kReverseSubtract:
        return D3D12_BLEND_OP_REV_SUBTRACT;
    case BlendOp::kMin:
        return D3D12_BLEND_OP_MIN;
    case BlendOp::kMax:
        return D3D12_BLEND_OP_MAX;
    default:
        NOTREACHED();
    }
}

D3D12_BLEND ConvertBlendOp(BlendFactor factor)
{
    switch (factor) {
    case BlendFactor::kZero:
        return D3D12_BLEND_ZERO;
    case BlendFactor::kOne:
        return D3D12_BLEND_ONE;
    case BlendFactor::kSrcColor:
        return D3D12_BLEND_SRC_COLOR;
    case BlendFactor::kOneMinusSrcColor:
        return D3D12_BLEND_INV_SRC_COLOR;
    case BlendFactor::kDstColor:
        return D3D12_BLEND_DEST_COLOR;
    case BlendFactor::kOneMinusDstColor:
        return D3D12_BLEND_INV_DEST_COLOR;
    case BlendFactor::kSrcAlpha:
        return D3D12_BLEND_SRC_ALPHA;
    case BlendFactor::kOneMinusSrcAlpha:
        return D3D12_BLEND_INV_SRC_ALPHA;
    case BlendFactor::kDstAlpha:
        return D3D12_BLEND_DEST_ALPHA;
    case BlendFactor::kOneMinusDstAlpha:
        return D3D12_BLEND_INV_DEST_ALPHA;
    case BlendFactor::kConstantColor:
        return D3D12_BLEND_BLEND_FACTOR;
    case BlendFactor::kOneMinusConstantColor:
        return D3D12_BLEND_INV_BLEND_FACTOR;
    case BlendFactor::kConstantAlpha:
        return D3D12_BLEND_ALPHA_FACTOR;
    case BlendFactor::kOneMinusConstantAlpha:
        return D3D12_BLEND_INV_ALPHA_FACTOR;
    case BlendFactor::kSrcAlphaSaturate:
        return D3D12_BLEND_SRC_ALPHA_SAT;
    case BlendFactor::kSrc1Color:
        return D3D12_BLEND_SRC1_COLOR;
    case BlendFactor::kOneMinusSrc1Color:
        return D3D12_BLEND_INV_SRC1_COLOR;
    case BlendFactor::kSrc1Alpha:
        return D3D12_BLEND_SRC1_ALPHA;
    case BlendFactor::kOneMinusSrc1Alpha:
        return D3D12_BLEND_INV_SRC1_ALPHA;
    default:
        NOTREACHED();
    }
}

CD3DX12_BLEND_DESC GetBlendDesc(const BlendDesc& desc)
{
    CD3DX12_BLEND_DESC blend_desc(D3D12_DEFAULT);
    blend_desc.IndependentBlendEnable = false;
    auto& render_target_blend_desc = blend_desc.RenderTarget[0];
    render_target_blend_desc.BlendEnable = desc.blend_enable;
    render_target_blend_desc.BlendOp = ConvertBlendOp(desc.color_blend_op);
    render_target_blend_desc.SrcBlend = ConvertBlendOp(desc.src_color_blend_factor);
    render_target_blend_desc.DestBlend = ConvertBlendOp(desc.dst_color_blend_factor);
    render_target_blend_desc.BlendOpAlpha = ConvertBlendOp(desc.alpha_blend_op);
    render_target_blend_desc.SrcBlendAlpha = ConvertBlendOp(desc.src_alpha_blend_factor);
    render_target_blend_desc.DestBlendAlpha = ConvertBlendOp(desc.dst_alpha_blend_factor);
    render_target_blend_desc.RenderTargetWriteMask = {};
    if (desc.color_write_mask & ColorComponentFlagBits::kRed) {
        render_target_blend_desc.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_RED;
    }
    if (desc.color_write_mask & ColorComponentFlagBits::kGreen) {
        render_target_blend_desc.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
    }
    if (desc.color_write_mask & ColorComponentFlagBits::kBlue) {
        render_target_blend_desc.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
    }
    if (desc.color_write_mask & ColorComponentFlagBits::kAlpha) {
        render_target_blend_desc.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
    }
    return blend_desc;
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
        NOTREACHED();
    }
}

D3D12_DEPTH_STENCILOP_DESC Convert(const StencilOpDesc& desc)
{
    D3D12_DEPTH_STENCILOP_DESC res = {};
    res.StencilFailOp = Convert(desc.fail_op);
    res.StencilPassOp = Convert(desc.pass_op);
    res.StencilDepthFailOp = Convert(desc.depth_fail_op);
    res.StencilFunc = ConvertToComparisonFunc(desc.func);
    return res;
}

CD3DX12_DEPTH_STENCIL_DESC1 GetDepthStencilDesc(const DepthStencilDesc& desc, DXGI_FORMAT dsv_format)
{
    CD3DX12_DEPTH_STENCIL_DESC1 depth_stencil_desc(D3D12_DEFAULT);
    depth_stencil_desc.DepthEnable = desc.depth_test_enable;
    depth_stencil_desc.DepthWriteMask =
        desc.depth_write_enable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    depth_stencil_desc.DepthFunc = ConvertToComparisonFunc(desc.depth_func);
    depth_stencil_desc.StencilEnable = desc.stencil_enable;
    depth_stencil_desc.StencilReadMask = desc.stencil_read_mask;
    depth_stencil_desc.StencilWriteMask = desc.stencil_write_mask;
    depth_stencil_desc.FrontFace = Convert(desc.front_face);
    depth_stencil_desc.BackFace = Convert(desc.back_face);
    depth_stencil_desc.DepthBoundsTestEnable = desc.depth_bounds_test_enable;
    return depth_stencil_desc;
}

D3D12_RT_FORMAT_ARRAY GetRTVFormats(const GraphicsPipelineDesc& desc)
{
    D3D12_RT_FORMAT_ARRAY rt_formats = {};
    for (size_t i = 0; i < desc.color_formats.size(); ++i) {
        if (desc.color_formats[i] == gli::format::FORMAT_UNDEFINED) {
            continue;
        }
        rt_formats.NumRenderTargets = i + 1;
        rt_formats.RTFormats[i] = static_cast<DXGI_FORMAT>(gli::dx().translate(desc.color_formats[i]).DXGIFormat.DDS);
    }
    return rt_formats;
}

DXGI_FORMAT GetDSVFormat(const GraphicsPipelineDesc& desc)
{
    if (desc.depth_stencil_format == gli::format::FORMAT_UNDEFINED) {
        return DXGI_FORMAT_UNKNOWN;
    }
    return static_cast<DXGI_FORMAT>(gli::dx().translate(desc.depth_stencil_format).DXGIFormat.DDS);
}

DXGI_SAMPLE_DESC GetSampleDesc(const GraphicsPipelineDesc& desc)
{
    return { desc.sample_count, 0 };
}

std::pair<std::string, uint32_t> SplitSemanticName(std::string semantic_name)
{
    uint32_t semantic_index = 0;
    uint32_t pow = 1;
    while (!semantic_name.empty() && std::isdigit(semantic_name.back())) {
        semantic_index = (semantic_name.back() - '0') * pow + semantic_index;
        semantic_name.pop_back();
        pow *= 10;
    }
    return { semantic_name, semantic_index };
}

} // namespace

DXGraphicsPipeline::DXGraphicsPipeline(DXDevice& device, const GraphicsPipelineDesc& desc)
    : device_(device)
    , desc_(desc)
{
    DXStateBuilder graphics_state_builder;

    decltype(auto) dx_layout = desc_.layout->As<DXBindingSetLayout>();
    root_signature_ = dx_layout.GetRootSignature();
    std::deque<std::string> semantic_names;
    for (const auto& shader : desc_.shaders) {
        decltype(auto) blob = shader->GetBlob();
        D3D12_SHADER_BYTECODE shader_bytecode = { blob.data(), blob.size() };

        switch (shader->GetType()) {
        case ShaderType::kVertex: {
            graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_VS>(shader_bytecode);
            ParseInputLayout(semantic_names);
            graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT>(GetInputLayoutDesc());
            graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT>(GetDSVFormat(desc));
            break;
        }
        case ShaderType::kGeometry:
            graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_GS>(shader_bytecode);
            break;
        case ShaderType::kAmplification:
            graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_AS>(shader_bytecode);
            break;
        case ShaderType::kMesh:
            graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_MS>(shader_bytecode);
            graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT>(GetDSVFormat(desc));
            break;
        case ShaderType::kPixel:
            graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_PS>(shader_bytecode);
            graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS>(GetRTVFormats(desc));
            break;
        default:
            break;
        }
    }

    graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1>(
        GetDepthStencilDesc(desc.depth_stencil_desc, GetDSVFormat(desc)));
    graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC>(GetSampleDesc(desc));
    graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER>(GetRasterizerDesc(desc.rasterizer_desc));
    graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC>(GetBlendDesc(desc.blend_desc));
    graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE>(root_signature_.Get());

    ComPtr<ID3D12Device2> device2;
    device_.GetDevice().As(&device2);
    auto pipeline_desc = graphics_state_builder.GetDesc();
    CHECK_HRESULT(device2->CreatePipelineState(&pipeline_desc, IID_PPV_ARGS(&pipeline_state_)));
}

void DXGraphicsPipeline::ParseInputLayout(std::deque<std::string>& semantic_names)
{
    for (const auto& vertex : desc_.input) {
        if (!input_layout_stride_.contains(vertex.slot)) {
            input_layout_stride_[vertex.slot] = vertex.stride;
        } else {
            CHECK(input_layout_stride_[vertex.slot] == vertex.stride);
        }

        auto [semantic_name, semantic_index] = SplitSemanticName(vertex.semantic_name);
        semantic_names.emplace_back(semantic_name);

        D3D12_INPUT_ELEMENT_DESC layout = {
            .SemanticName = semantic_names.back().c_str(),
            .SemanticIndex = semantic_index,
            .Format = static_cast<DXGI_FORMAT>(gli::dx().translate(vertex.format).DXGIFormat.DDS),
            .InputSlot = vertex.slot,
            .AlignedByteOffset = vertex.offset,
            .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            .InstanceDataStepRate = 0,
        };
        input_layout_desc_.push_back(layout);
    }
}

D3D12_INPUT_LAYOUT_DESC DXGraphicsPipeline::GetInputLayoutDesc()
{
    D3D12_INPUT_LAYOUT_DESC input_layout_desc = {};
    input_layout_desc.NumElements = static_cast<uint32_t>(input_layout_desc_.size());
    input_layout_desc.pInputElementDescs = input_layout_desc_.data();
    return input_layout_desc;
}

PipelineType DXGraphicsPipeline::GetPipelineType() const
{
    return PipelineType::kGraphics;
}

const GraphicsPipelineDesc& DXGraphicsPipeline::GetDesc() const
{
    return desc_;
}

const ComPtr<ID3D12PipelineState>& DXGraphicsPipeline::GetPipeline() const
{
    return pipeline_state_;
}

const ComPtr<ID3D12RootSignature>& DXGraphicsPipeline::GetRootSignature() const
{
    return root_signature_;
}

const std::map<size_t, uint32_t>& DXGraphicsPipeline::GetStrideMap() const
{
    return input_layout_stride_;
}

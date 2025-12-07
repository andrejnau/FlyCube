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
    for (size_t i = 0; i < desc.color_formats.size(); ++i) {
        if (desc.color_formats[i] == gli::format::FORMAT_UNDEFINED) {
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
    depth_stencil_desc.DepthBoundsTestEnable = desc.depth_bounds_test_enable;
    depth_stencil_desc.DepthFunc = ConvertToComparisonFunc(desc.depth_func);
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
    graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER>(GetRasterizerDesc(desc));
    graphics_state_builder.AddState<CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC>(GetBlendDesc(desc));
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

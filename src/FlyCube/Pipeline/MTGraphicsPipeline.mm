#include "Pipeline/MTGraphicsPipeline.h"

#include "Device/MTDevice.h"
#include "Shader/MTShader.h"
#include "Utilities/Check.h"
#include "Utilities/Logging.h"
#include "Utilities/NotReached.h"

namespace {

MTLCompareFunction ConvertCompareFunction(ComparisonFunc func)
{
    switch (func) {
    case ComparisonFunc::kNever:
        return MTLCompareFunctionNever;
    case ComparisonFunc::kLess:
        return MTLCompareFunctionLess;
    case ComparisonFunc::kEqual:
        return MTLCompareFunctionEqual;
    case ComparisonFunc::kLessEqual:
        return MTLCompareFunctionLessEqual;
    case ComparisonFunc::kGreater:
        return MTLCompareFunctionGreater;
    case ComparisonFunc::kNotEqual:
        return MTLCompareFunctionNotEqual;
    case ComparisonFunc::kGreaterEqual:
        return MTLCompareFunctionGreaterEqual;
    case ComparisonFunc::kAlways:
        return MTLCompareFunctionAlways;
    default:
        NOTREACHED();
    }
}

MTLStencilOperation ConvertStencilOperation(StencilOp op)
{
    switch (op) {
    case StencilOp::kKeep:
        return MTLStencilOperationKeep;
    case StencilOp::kZero:
        return MTLStencilOperationZero;
    case StencilOp::kReplace:
        return MTLStencilOperationReplace;
    case StencilOp::kIncrSat:
        return MTLStencilOperationIncrementClamp;
    case StencilOp::kDecrSat:
        return MTLStencilOperationDecrementClamp;
    case StencilOp::kInvert:
        return MTLStencilOperationInvert;
    case StencilOp::kIncr:
        return MTLStencilOperationIncrementWrap;
    case StencilOp::kDecr:
        return MTLStencilOperationDecrementWrap;
    default:
        NOTREACHED();
    }
}

MTLStencilDescriptor* GetStencilDesc(bool stencil_enable,
                                     const StencilOpDesc& desc,
                                     uint8_t read_mask,
                                     uint8_t write_mask)
{
    MTLStencilDescriptor* stencil_descriptor = [MTLStencilDescriptor new];
    if (stencil_enable) {
        stencil_descriptor.stencilCompareFunction = ConvertCompareFunction(desc.func);
    }
    stencil_descriptor.stencilFailureOperation = ConvertStencilOperation(desc.fail_op);
    stencil_descriptor.depthFailureOperation = ConvertStencilOperation(desc.depth_fail_op);
    stencil_descriptor.depthStencilPassOperation = ConvertStencilOperation(desc.pass_op);
    stencil_descriptor.readMask = read_mask;
    stencil_descriptor.writeMask = write_mask;
    return stencil_descriptor;
}

MTLDepthStencilDescriptor* GetDepthStencilDesc(const DepthStencilDesc& desc, gli::format depth_stencil_format)
{
    MTLDepthStencilDescriptor* depth_stencil_descriptor = [MTLDepthStencilDescriptor new];
    if (desc.depth_test_enable && depth_stencil_format != gli::format::FORMAT_UNDEFINED &&
        gli::is_depth(depth_stencil_format)) {
        depth_stencil_descriptor.depthCompareFunction = ConvertCompareFunction(desc.depth_func);
    }
    depth_stencil_descriptor.depthWriteEnabled = desc.depth_write_enable;

    const bool stencil_enable = desc.stencil_enable && depth_stencil_format != gli::format::FORMAT_UNDEFINED &&
                                gli::is_stencil(depth_stencil_format);
    depth_stencil_descriptor.frontFaceStencil =
        GetStencilDesc(stencil_enable, desc.front_face, desc.stencil_read_mask, desc.stencil_write_mask);
    depth_stencil_descriptor.backFaceStencil =
        GetStencilDesc(stencil_enable, desc.back_face, desc.stencil_read_mask, desc.stencil_write_mask);
    return depth_stencil_descriptor;
}

} // namespace

MTGraphicsPipeline::MTGraphicsPipeline(MTDevice& device, const GraphicsPipelineDesc& desc)
    : device_(device)
    , desc_(desc)
{
    for (const auto& shader : desc.shaders) {
        shader_by_type_[shader->GetType()] = shader;
    }

    if (shader_by_type_.contains(ShaderType::kAmplification) || shader_by_type_.contains(ShaderType::kMesh)) {
        CreatePipeline</*is_mesh_pipeline=*/true>();
    } else {
        CreatePipeline</*is_mesh_pipeline=*/false>();
    }
}

template <bool is_mesh_pipeline>
void MTGraphicsPipeline::CreatePipeline()
{
    using RenderPipelineDescriptorType =
        std::conditional_t<is_mesh_pipeline, MTL4MeshRenderPipelineDescriptor, MTL4RenderPipelineDescriptor>;
    RenderPipelineDescriptorType* pipeline_descriptor = [RenderPipelineDescriptorType new];
    for (const auto& shader : desc_.shaders) {
        MTL4LibraryFunctionDescriptor* function_descriptor = shader->As<MTShader>().GetFunctionDescriptor();
        switch (shader->GetType()) {
        case ShaderType::kVertex:
            if constexpr (!is_mesh_pipeline) {
                pipeline_descriptor.vertexFunctionDescriptor = function_descriptor;
                pipeline_descriptor.vertexDescriptor = GetVertexDescriptor(shader);
            }
            break;
        case ShaderType::kPixel:
            pipeline_descriptor.fragmentFunctionDescriptor = function_descriptor;
            break;
        case ShaderType::kAmplification:
            if constexpr (is_mesh_pipeline) {
                pipeline_descriptor.objectFunctionDescriptor = function_descriptor;
                decltype(auto) numthreads = shader->GetReflection()->GetShaderFeatureInfo().numthreads;
                amplification_numthreads_ = { numthreads[0], numthreads[1], numthreads[2] };
            }
            break;
        case ShaderType::kMesh:
            if constexpr (is_mesh_pipeline) {
                pipeline_descriptor.meshFunctionDescriptor = function_descriptor;
                decltype(auto) numthreads = shader->GetReflection()->GetShaderFeatureInfo().numthreads;
                mesh_numthreads_ = { numthreads[0], numthreads[1], numthreads[2] };
            }
            break;
        default:
            NOTREACHED();
        }
    }

    for (size_t i = 0; i < desc_.color_formats.size(); ++i) {
        if (desc_.color_formats[i] == gli::format::FORMAT_UNDEFINED) {
            continue;
        }
        pipeline_descriptor.colorAttachments[i].pixelFormat = device_.GetMTLPixelFormat(desc_.color_formats[i]);
    }
    if constexpr (!is_mesh_pipeline) {
        pipeline_descriptor.inputPrimitiveTopology = MTLPrimitiveTopologyClassTriangle;
    }
    pipeline_descriptor.rasterSampleCount = desc_.sample_count;

    decltype(auto) blend_desc = desc_.blend_desc;
    for (size_t i = 0; i < desc_.color_formats.size(); ++i) {
        if (desc_.color_formats[i] == gli::format::FORMAT_UNDEFINED) {
            continue;
        }

        decltype(auto) attachment = pipeline_descriptor.colorAttachments[i];
        attachment.blendingState = blend_desc.blend_enable ? MTL4BlendStateEnabled : MTL4BlendStateDisabled;
        if (!blend_desc.blend_enable) {
            continue;
        }

        auto convert = [](Blend type) {
            switch (type) {
            case Blend::kZero:
                return MTLBlendFactorZero;
            case Blend::kSrcAlpha:
                return MTLBlendFactorSourceAlpha;
            case Blend::kInvSrcAlpha:
                return MTLBlendFactorOneMinusSourceAlpha;
            default:
                NOTREACHED();
            }
        };

        auto convert_op = [](BlendOp type) {
            switch (type) {
            case BlendOp::kAdd:
                return MTLBlendOperationAdd;
            default:
                NOTREACHED();
            }
        };

        attachment.sourceRGBBlendFactor = convert(desc_.blend_desc.blend_src);
        attachment.destinationRGBBlendFactor = convert(desc_.blend_desc.blend_dest);
        attachment.rgbBlendOperation = convert_op(desc_.blend_desc.blend_op);
        attachment.sourceAlphaBlendFactor = convert(desc_.blend_desc.blend_src_alpha);
        attachment.destinationAlphaBlendFactor = convert(desc_.blend_desc.blend_dest_apha);
        attachment.alphaBlendOperation = convert_op(desc_.blend_desc.blend_op_alpha);
    }

    NSError* error = nullptr;
    pipeline_ = [device_.GetCompiler() newRenderPipelineStateWithDescriptor:pipeline_descriptor
                                                        compilerTaskOptions:nullptr
                                                                      error:&error];
    if (!pipeline_) {
        Logging::Println("Failed to create MTLRenderPipelineState: {}", error);
    }

    depth_stencil_ = [device_.GetDevice()
        newDepthStencilStateWithDescriptor:GetDepthStencilDesc(desc_.depth_stencil_desc, desc_.depth_stencil_format)];
}

MTLVertexDescriptor* MTGraphicsPipeline::GetVertexDescriptor(const std::shared_ptr<Shader>& shader)
{
    MTLVertexDescriptor* vertex_descriptor = [MTLVertexDescriptor new];
    std::map<size_t, uint32_t> input_layout_stride;
    for (const auto& vertex : desc_.input) {
        if (!input_layout_stride.contains(vertex.slot)) {
            input_layout_stride[vertex.slot] = vertex.stride;
        } else {
            CHECK(input_layout_stride[vertex.slot] == vertex.stride);
        }

        const uint32_t buffer_index = device_.GetMaxPerStageBufferCount() - vertex.slot - 1;
        MTLVertexBufferLayoutDescriptor* layout = vertex_descriptor.layouts[buffer_index];
        layout.stride = vertex.stride;
        layout.stepFunction = MTLVertexStepFunctionPerVertex;
        layout.stepRate = 1;

        const uint32_t location = shader->GetInputLayoutLocation(vertex.semantic_name);
        MTLVertexAttributeDescriptor* attribute = vertex_descriptor.attributes[location];
        attribute.offset = vertex.offset;
        attribute.bufferIndex = buffer_index;
        attribute.format = device_.GetMTLVertexFormat(vertex.format);
    }
    return vertex_descriptor;
}

PipelineType MTGraphicsPipeline::GetPipelineType() const
{
    return PipelineType::kGraphics;
}

std::vector<uint8_t> MTGraphicsPipeline::GetRayTracingShaderGroupHandles(uint32_t first_group,
                                                                         uint32_t group_count) const
{
    NOTREACHED();
}

std::shared_ptr<Shader> MTGraphicsPipeline::GetShader(ShaderType type) const
{
    if (auto it = shader_by_type_.find(type); it != shader_by_type_.end()) {
        return it->second;
    }
    return nullptr;
}

id<MTLRenderPipelineState> MTGraphicsPipeline::GetPipeline()
{
    return pipeline_;
}

id<MTLDepthStencilState> MTGraphicsPipeline::GetDepthStencil()
{
    return depth_stencil_;
}

const GraphicsPipelineDesc& MTGraphicsPipeline::GetDesc() const
{
    return desc_;
}

const MTLSize& MTGraphicsPipeline::GetAmplificationNumthreads() const
{
    return amplification_numthreads_;
}

const MTLSize& MTGraphicsPipeline::GetMeshNumthreads() const
{
    return mesh_numthreads_;
}

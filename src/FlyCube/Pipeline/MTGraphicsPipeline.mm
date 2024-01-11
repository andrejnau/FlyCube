#include "Pipeline/MTGraphicsPipeline.h"

#include "Device/MTDevice.h"
#include "Shader/MTShader.h"

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
        assert(false);
        return MTLCompareFunctionLess;
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
        assert(false);
        return MTLStencilOperationKeep;
    }
}

MTLStencilDescriptor* GetStencilDesc(const StencilOpDesc& desc, uint8_t read_mask, uint8_t write_mask)
{
    MTLStencilDescriptor* stencil_descriptor = [[MTLStencilDescriptor alloc] init];
    stencil_descriptor.stencilCompareFunction = ConvertCompareFunction(desc.func);
    stencil_descriptor.stencilFailureOperation = ConvertStencilOperation(desc.fail_op);
    stencil_descriptor.depthFailureOperation = ConvertStencilOperation(desc.depth_fail_op);
    stencil_descriptor.depthStencilPassOperation = ConvertStencilOperation(desc.pass_op);
    stencil_descriptor.readMask = read_mask;
    stencil_descriptor.writeMask = write_mask;
    return stencil_descriptor;
}

MTLDepthStencilDescriptor* GetDepthStencilDesc(const DepthStencilDesc& desc, gli::format depth_stencil_format)
{
    MTLDepthStencilDescriptor* depth_stencil_descriptor = [[MTLDepthStencilDescriptor alloc] init];
    depth_stencil_descriptor.depthCompareFunction = ConvertCompareFunction(desc.depth_func);
    depth_stencil_descriptor.depthWriteEnabled = desc.depth_write_enable;
    if (depth_stencil_format == gli::format::FORMAT_UNDEFINED || !gli::is_depth(depth_stencil_format)) {
        depth_stencil_descriptor.depthCompareFunction = MTLCompareFunctionAlways;
        depth_stencil_descriptor.depthWriteEnabled = false;
    }

    depth_stencil_descriptor.frontFaceStencil =
        GetStencilDesc(desc.front_face, desc.stencil_read_mask, desc.stencil_write_mask);
    depth_stencil_descriptor.backFaceStencil =
        GetStencilDesc(desc.back_face, desc.stencil_read_mask, desc.stencil_write_mask);
    return depth_stencil_descriptor;
}

} // namespace

MTGraphicsPipeline::MTGraphicsPipeline(MTDevice& device, const GraphicsPipelineDesc& desc)
    : m_device(device)
    , m_desc(desc)
{
    decltype(auto) mt_device = device.GetDevice();
    MTLRenderPipelineDescriptor* pipeline_descriptor = [[MTLRenderPipelineDescriptor alloc] init];
    decltype(auto) shaders = desc.program->GetShaders();

    for (const auto& shader : shaders) {
        decltype(auto) mt_shader = shader->As<MTShader>();
        id<MTLLibrary> library = mt_shader.GetLibrary();
        decltype(auto) reflection = shader->GetReflection();
        for (const auto& entry_point : reflection->GetEntryPoints()) {
            id<MTLFunction> function = mt_shader.CreateFunction(library, entry_point.name);
            switch (shader->GetType()) {
            case ShaderType::kVertex:
                pipeline_descriptor.vertexFunction = function;
                pipeline_descriptor.vertexDescriptor = GetVertexDescriptor(shader);
                break;
            case ShaderType::kPixel:
                pipeline_descriptor.fragmentFunction = function;
                break;
            default:
                assert(false);
                break;
            }
        }
    }

    MVKPixelFormats& pixel_formats = m_device.GetMVKPixelFormats();
    const RenderPassDesc& render_pass_desc = desc.render_pass->GetDesc();
    for (size_t i = 0; i < render_pass_desc.colors.size(); ++i) {
        if (render_pass_desc.colors[i].format == gli::format::FORMAT_UNDEFINED) {
            continue;
        }
        pipeline_descriptor.colorAttachments[i].pixelFormat =
            pixel_formats.getMTLPixelFormat(static_cast<VkFormat>(render_pass_desc.colors[i].format));
    }
    decltype(auto) depth_stencil_format = render_pass_desc.depth_stencil.format;
    if (depth_stencil_format != gli::format::FORMAT_UNDEFINED) {
        if (gli::is_depth(depth_stencil_format)) {
            pipeline_descriptor.depthAttachmentPixelFormat =
                pixel_formats.getMTLPixelFormat(static_cast<VkFormat>(depth_stencil_format));
        }
        if (gli::is_stencil(depth_stencil_format)) {
            pipeline_descriptor.stencilAttachmentPixelFormat =
                pixel_formats.getMTLPixelFormat(static_cast<VkFormat>(depth_stencil_format));
        }
    }
    pipeline_descriptor.inputPrimitiveTopology = MTLPrimitiveTopologyClassTriangle;
    pipeline_descriptor.rasterSampleCount = render_pass_desc.sample_count;

    if (pipeline_descriptor.depthAttachmentPixelFormat == MTLPixelFormatInvalid &&
        pipeline_descriptor.stencilAttachmentPixelFormat == MTLPixelFormatInvalid &&
        pipeline_descriptor.colorAttachments[0].pixelFormat == MTLPixelFormatInvalid) {
        pipeline_descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatR8Unorm;
    }

    decltype(auto) blend_desc = m_desc.blend_desc;
    for (size_t i = 0; i < render_pass_desc.colors.size(); ++i) {
        if (render_pass_desc.colors[i].format == gli::format::FORMAT_UNDEFINED) {
            continue;
        }

        decltype(auto) attachment = pipeline_descriptor.colorAttachments[i];
        attachment.blendingEnabled = blend_desc.blend_enable;
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
            }
            throw std::runtime_error("unsupported");
        };

        auto convert_op = [](BlendOp type) {
            switch (type) {
            case BlendOp::kAdd:
                return MTLBlendOperationAdd;
            }
            throw std::runtime_error("unsupported");
        };

        attachment.sourceRGBBlendFactor = convert(m_desc.blend_desc.blend_src);
        attachment.destinationRGBBlendFactor = convert(m_desc.blend_desc.blend_dest);
        attachment.rgbBlendOperation = convert_op(m_desc.blend_desc.blend_op);
        attachment.sourceAlphaBlendFactor = convert(m_desc.blend_desc.blend_src_alpha);
        attachment.destinationAlphaBlendFactor = convert(m_desc.blend_desc.blend_dest_apha);
        attachment.alphaBlendOperation = convert_op(m_desc.blend_desc.blend_op_alpha);
    }

    NSError* error = nullptr;
    m_pipeline = [mt_device newRenderPipelineStateWithDescriptor:pipeline_descriptor error:&error];
    if (!m_pipeline) {
        NSLog(@"Error occurred when creating render pipeline state: %@", error);
    }

    m_depth_stencil = [m_device.GetDevice()
        newDepthStencilStateWithDescriptor:GetDepthStencilDesc(desc.depth_stencil_desc, depth_stencil_format)];
}

MTLVertexDescriptor* MTGraphicsPipeline::GetVertexDescriptor(const std::shared_ptr<Shader>& shader)
{
    MVKPixelFormats& pixel_formats = m_device.GetMVKPixelFormats();
    MTLVertexDescriptor* vertex_descriptor = [MTLVertexDescriptor new];
    for (size_t i = 0; i < m_desc.input.size(); ++i) {
        decltype(auto) vertex = m_desc.input[i];
        decltype(auto) attribute = vertex_descriptor.attributes[i];
        attribute.offset = 0;
        attribute.bufferIndex = m_device.GetMaxPerStageBufferCount() - vertex.slot - 1;
        attribute.format = pixel_formats.getMTLVertexFormat(static_cast<VkFormat>(vertex.format));
        decltype(auto) layout = vertex_descriptor.layouts[attribute.bufferIndex];
        layout.stride = vertex.stride;
        layout.stepFunction = MTLVertexStepFunctionPerVertex;
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
    assert(false);
    return {};
}

std::shared_ptr<Program> MTGraphicsPipeline::GetProgram() const
{
    return m_desc.program;
}

id<MTLRenderPipelineState> MTGraphicsPipeline::GetPipeline()
{
    return m_pipeline;
}

id<MTLDepthStencilState> MTGraphicsPipeline::GetDepthStencil()
{
    return m_depth_stencil;
}

const GraphicsPipelineDesc& MTGraphicsPipeline::GetDesc() const
{
    return m_desc;
}

#include "Pipeline/MTGraphicsPipeline.h"
#include <Device/MTDevice.h>
#include <HLSLCompiler/MSLConverter.h>

std::string FixEntryPoint(const std::string& entry_point)
{
    if (entry_point == "main")
        return "main0";
    return entry_point;
}

MTGraphicsPipeline::MTGraphicsPipeline(MTDevice& device, const GraphicsPipelineDesc& desc)
    : m_device(device)
    , m_desc(desc)
{
    decltype(auto) mtl_device = device.GetDevice();
    MTLRenderPipelineDescriptor* pipeline_descriptor = [[MTLRenderPipelineDescriptor alloc] init];
    NSError* error = nil;

    decltype(auto) shaders = desc.program->GetShaders();
    for (const auto& shader : shaders)
    {
        decltype(auto) blob = shader->GetBlob();
        decltype(auto) source = GetMSLShader(blob);
        NSString* ns_source = [NSString stringWithUTF8String:source.c_str()];

        id<MTLLibrary> library = [mtl_device newLibraryWithSource:ns_source options:nil error:&error];
        if (library == nil)
        {
            NSLog(@"Error: failed to create Metal library: %@", error);
            continue;
        }

        decltype(auto) reflection = shader->GetReflection();
        decltype(auto) entry_points = reflection->GetEntryPoints();
        for (const auto& entry_point : entry_points)
        {
            NSString* ns_entry_point = [NSString stringWithUTF8String:FixEntryPoint(entry_point.name).c_str()];
            MTLFunctionConstantValues* constant_values = [MTLFunctionConstantValues new];
            id<MTLFunction> function = [library newFunctionWithName:ns_entry_point constantValues:constant_values error:&error];
            if (function == nil)
            {
                NSLog(@"Error: failed to create Metal function: %@", error);
                continue;
            }
            
            switch (shader->GetType())
            {
            case ShaderType::kVertex:
                pipeline_descriptor.vertexFunction = function;
                pipeline_descriptor.vertexDescriptor = GetVertexDescriptor(shader);
                break;
            case ShaderType::kPixel:
                pipeline_descriptor.fragmentFunction = function;
                break;
            default:
                break;
            }
        }
    }
    
    MVKPixelFormats& pixel_formats = m_device.GetMVKPixelFormats();
    const RenderPassDesc& render_pass_desc = desc.render_pass->GetDesc();
    for (size_t i = 0; i < render_pass_desc.colors.size(); ++i)
    {
        if (render_pass_desc.colors[i].format == gli::format::FORMAT_UNDEFINED)
            continue;
        pipeline_descriptor.colorAttachments[i].pixelFormat = pixel_formats.getMTLPixelFormat(static_cast<VkFormat>(render_pass_desc.colors[i].format));
    }
    
    m_pipeline = [mtl_device newRenderPipelineStateWithDescriptor:pipeline_descriptor error:&error];
    if (!m_pipeline)
    {
        NSLog(@"Error occurred when creating render pipeline state: %@", error);
    }
}

MTLVertexDescriptor* MTGraphicsPipeline::GetVertexDescriptor(const std::shared_ptr<Shader>& shader)
{
    MVKPixelFormats& pixel_formats = m_device.GetMVKPixelFormats();
    MTLVertexDescriptor* vertex_descriptor = [MTLVertexDescriptor new];
    for (size_t i = 0; i < m_desc.input.size(); ++i)
    {
        decltype(auto) vertex = m_desc.input[i];
        decltype(auto) attribute = vertex_descriptor.attributes[i];
        decltype(auto) layout = vertex_descriptor.layouts[i];
        attribute.offset = 0;
        attribute.bufferIndex = vertex.slot;
        attribute.format = pixel_formats.getMTLVertexFormat(static_cast<VkFormat>(vertex.format));
        layout.stride = vertex.stride;
        layout.stepFunction = MTLVertexStepFunctionPerVertex;
    }
    return vertex_descriptor;
}

PipelineType MTGraphicsPipeline::GetPipelineType() const
{
    return PipelineType::kGraphics;
}

std::vector<uint8_t> MTGraphicsPipeline::GetRayTracingShaderGroupHandles(uint32_t first_group, uint32_t group_count) const
{
    return {};
}

id<MTLRenderPipelineState> MTGraphicsPipeline::GetPipeline()
{
    return m_pipeline;
}

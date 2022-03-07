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
    : m_desc(desc)
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
                break;
            case ShaderType::kPixel:
                pipeline_descriptor.fragmentFunction = function;
                break;
            }
        }
    }
    
    auto pipeline = [mtl_device newRenderPipelineStateWithDescriptor:pipeline_descriptor error:&error];
    if (!pipeline)
    {
        NSLog(@"Error occurred when creating render pipeline state: %@", error);
    }
}

PipelineType MTGraphicsPipeline::GetPipelineType() const
{
    return PipelineType::kGraphics;
}

std::vector<uint8_t> MTGraphicsPipeline::GetRayTracingShaderGroupHandles(uint32_t first_group, uint32_t group_count) const
{
    return {};
}

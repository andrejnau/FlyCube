#include "Pipeline/MTComputePipeline.h"
#include <Device/MTDevice.h>
#include <Shader/MTShader.h>

static std::string FixEntryPoint(const std::string& entry_point)
{
    if (entry_point == "main")
        return "main0";
    return entry_point;
}

MTComputePipeline::MTComputePipeline(MTDevice& device, const ComputePipelineDesc& desc)
    : m_device(device)
    , m_desc(desc)
{
    decltype(auto) mt_device = device.GetDevice();
    MTLComputePipelineDescriptor* pipeline_descriptor = [[MTLComputePipelineDescriptor alloc] init];
    NSError* error = nil;

    decltype(auto) shaders = desc.program->GetShaders();
    for (const auto& shader : shaders)
    {
        decltype(auto) source = shader->As<MTShader>().GetSource();
        NSString* ns_source = [NSString stringWithUTF8String:source.c_str()];

        id<MTLLibrary> library = [mt_device newLibraryWithSource:ns_source
                                                          options:nil
                                                            error:&error];
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
            id<MTLFunction> function = [library newFunctionWithName:ns_entry_point
                                                     constantValues:constant_values
                                                              error:&error];
            if (function == nil)
            {
                NSLog(@"Error: failed to create Metal function: %@", error);
                continue;
            }
            
            switch (shader->GetType())
            {
            case ShaderType::kCompute:
                pipeline_descriptor.computeFunction = function;
                break;
            default:
                break;
            }
        }
        decltype(auto) numthreads = reflection->GetShaderFeatureInfo().numthreads;
        m_numthreads = {numthreads[0], numthreads[1], numthreads[2]};
    }
    
    MTLPipelineOption opt = MTLPipelineOptionNone;
    m_pipeline = [mt_device newComputePipelineStateWithDescriptor:pipeline_descriptor
                                                          options:opt
                                                       reflection:nil
                                                            error:&error];
    if (!m_pipeline)
    {
        NSLog(@"Error occurred when creating render pipeline state: %@", error);
    }
}

PipelineType MTComputePipeline::GetPipelineType() const
{
    return PipelineType::kCompute;
}

std::vector<uint8_t> MTComputePipeline::GetRayTracingShaderGroupHandles(uint32_t first_group, uint32_t group_count) const
{
    return {};
}

std::shared_ptr<Program> MTComputePipeline::GetProgram() const
{
    return m_desc.program;
}

id<MTLComputePipelineState> MTComputePipeline::GetPipeline()
{
    return m_pipeline;
}

const ComputePipelineDesc& MTComputePipeline::GetDesc() const
{
    return m_desc;
}

const MTLSize MTComputePipeline::GetNumthreads() const
{
    return m_numthreads;
}

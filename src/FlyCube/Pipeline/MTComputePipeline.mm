#include "Pipeline/MTComputePipeline.h"

#include "Device/MTDevice.h"
#include "Shader/MTShader.h"

MTComputePipeline::MTComputePipeline(MTDevice& device, const ComputePipelineDesc& desc)
    : m_device(device)
    , m_desc(desc)
{
    decltype(auto) mt_device = device.GetDevice();
    MTLComputePipelineDescriptor* pipeline_descriptor = [[MTLComputePipelineDescriptor alloc] init];
    decltype(auto) shaders = desc.program->GetShaders();

    for (const auto& shader : shaders) {
        decltype(auto) mt_shader = shader->As<MTShader>();
        id<MTLLibrary> library = mt_shader.GetLibrary();
        decltype(auto) reflection = shader->GetReflection();
        for (const auto& entry_point : reflection->GetEntryPoints()) {
            id<MTLFunction> function = mt_shader.CreateFunction(library, entry_point.name);
            switch (shader->GetType()) {
            case ShaderType::kCompute:
                pipeline_descriptor.computeFunction = function;
                break;
            default:
                assert(false);
                break;
            }
        }
        decltype(auto) numthreads = reflection->GetShaderFeatureInfo().numthreads;
        m_numthreads = { numthreads[0], numthreads[1], numthreads[2] };
    }

    MTLPipelineOption opt = MTLPipelineOptionNone;
    NSError* error = nullptr;
    m_pipeline = [mt_device newComputePipelineStateWithDescriptor:pipeline_descriptor
                                                          options:opt
                                                       reflection:nullptr
                                                            error:&error];
    if (!m_pipeline) {
        NSLog(@"Error occurred when creating render pipeline state: %@", error);
    }
}

PipelineType MTComputePipeline::GetPipelineType() const
{
    return PipelineType::kCompute;
}

std::vector<uint8_t> MTComputePipeline::GetRayTracingShaderGroupHandles(uint32_t first_group,
                                                                        uint32_t group_count) const
{
    assert(false);
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

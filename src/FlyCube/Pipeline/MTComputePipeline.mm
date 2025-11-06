#include "Pipeline/MTComputePipeline.h"

#include "Device/MTDevice.h"
#include "Shader/MTShader.h"
#include "Utilities/Logging.h"
#include "Utilities/NotReached.h"

MTComputePipeline::MTComputePipeline(MTDevice& device, const ComputePipelineDesc& desc)
    : m_device(device)
    , m_desc(desc)
{
    decltype(auto) mt_device = device.GetDevice();
    MTL4ComputePipelineDescriptor* pipeline_descriptor = [MTL4ComputePipelineDescriptor new];
    decltype(auto) shaders = desc.program->GetShaders();
    for (const auto& shader : shaders) {
        decltype(auto) mt_shader = shader->As<MTShader>();
        decltype(auto) reflection = shader->GetReflection();
        for (const auto& entry_point : reflection->GetEntryPoints()) {
            MTL4LibraryFunctionDescriptor* function_descriptor = mt_shader.CreateFunctionDescriptor(entry_point.name);
            switch (shader->GetType()) {
            case ShaderType::kCompute:
                pipeline_descriptor.computeFunctionDescriptor = function_descriptor;
                break;
            default:
                NOTREACHED();
            }
        }
        decltype(auto) numthreads = reflection->GetShaderFeatureInfo().numthreads;
        m_numthreads = { numthreads[0], numthreads[1], numthreads[2] };
    }

    NSError* error = nullptr;
    m_pipeline = [device.GetCompiler() newComputePipelineStateWithDescriptor:pipeline_descriptor
                                                         compilerTaskOptions:nullptr
                                                                       error:&error];
    if (!m_pipeline) {
        Logging::Println("Failed to create MTLComputePipelineState: {}", error);
    }
}

PipelineType MTComputePipeline::GetPipelineType() const
{
    return PipelineType::kCompute;
}

std::vector<uint8_t> MTComputePipeline::GetRayTracingShaderGroupHandles(uint32_t first_group,
                                                                        uint32_t group_count) const
{
    NOTREACHED();
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

const MTLSize& MTComputePipeline::GetNumthreads() const
{
    return m_numthreads;
}

#include "Pipeline/MTComputePipeline.h"

#include "Device/MTDevice.h"
#include "Shader/MTShader.h"
#include "Utilities/Logging.h"
#include "Utilities/NotReached.h"

#include <cassert>

MTComputePipeline::MTComputePipeline(MTDevice& device, const ComputePipelineDesc& desc)
    : m_device(device)
    , m_desc(desc)
{
    decltype(auto) mt_device = device.GetDevice();
    MTL4ComputePipelineDescriptor* pipeline_descriptor = [MTL4ComputePipelineDescriptor new];

    decltype(auto) mt_shader = m_desc.shader->As<MTShader>();
    assert(mt_shader.GetType() == ShaderType::kCompute);
    decltype(auto) reflection = mt_shader.GetReflection();
    auto entry_point = reflection->GetEntryPoints().at(0);
    MTL4LibraryFunctionDescriptor* function_descriptor = mt_shader.CreateFunctionDescriptor(entry_point.name);
    pipeline_descriptor.computeFunctionDescriptor = function_descriptor;
    decltype(auto) numthreads = reflection->GetShaderFeatureInfo().numthreads;
    m_numthreads = { numthreads[0], numthreads[1], numthreads[2] };

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

std::shared_ptr<Shader> MTComputePipeline::GetShader(ShaderType type) const
{
    if (type == ShaderType::kCompute) {
        return m_desc.shader;
    }
    return nullptr;
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

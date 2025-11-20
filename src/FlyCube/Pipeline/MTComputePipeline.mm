#include "Pipeline/MTComputePipeline.h"

#include "Device/MTDevice.h"
#include "Shader/MTShader.h"
#include "Utilities/Logging.h"
#include "Utilities/NotReached.h"

#include <cassert>

MTComputePipeline::MTComputePipeline(MTDevice& device, const ComputePipelineDesc& desc)
    : desc_(desc)
{
    MTL4ComputePipelineDescriptor* pipeline_descriptor = [MTL4ComputePipelineDescriptor new];
    assert(desc_.shader->GetType() == ShaderType::kCompute);
    pipeline_descriptor.computeFunctionDescriptor = desc_.shader->As<MTShader>().GetFunctionDescriptor();

    decltype(auto) reflection = desc_.shader->GetReflection();
    decltype(auto) numthreads = reflection->GetShaderFeatureInfo().numthreads;
    numthreads_ = { numthreads[0], numthreads[1], numthreads[2] };

    NSError* error = nullptr;
    pipeline_ = [device.GetCompiler() newComputePipelineStateWithDescriptor:pipeline_descriptor
                                                        compilerTaskOptions:nullptr
                                                                      error:&error];
    if (!pipeline_) {
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
        return desc_.shader;
    }
    return nullptr;
}

id<MTLComputePipelineState> MTComputePipeline::GetPipeline()
{
    return pipeline_;
}

const ComputePipelineDesc& MTComputePipeline::GetDesc() const
{
    return desc_;
}

const MTLSize& MTComputePipeline::GetNumthreads() const
{
    return numthreads_;
}

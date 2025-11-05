#include "BindingSet/MTBindingSet.h"

#include "BindingSetLayout/MTBindingSetLayout.h"
#include "Device/MTDevice.h"
#include "HLSLCompiler/MSLConverter.h"
#include "Pipeline/MTPipeline.h"
#include "Shader/MTShader.h"
#include "Utilities/NotReached.h"
#include "View/MTView.h"

namespace {

void SetBuffer(id<MTL4ArgumentTable> argument_table, id<MTLBuffer> buffer, uint32_t offset, uint32_t index)
{
    [argument_table setAddress:buffer.gpuAddress + offset atIndex:index];
}

void SetSamplerState(id<MTL4ArgumentTable> argument_table, id<MTLSamplerState> sampler, uint32_t index)
{
    [argument_table setSamplerState:sampler.gpuResourceID atIndex:index];
}

void SetTexture(id<MTL4ArgumentTable> argument_table, id<MTLTexture> texture, uint32_t index)
{
    [argument_table setTexture:texture.gpuResourceID atIndex:index];
}

void SetAccelerationStructure(id<MTL4ArgumentTable> argument_table,
                              id<MTLAccelerationStructure> acceleration_structure,
                              uint32_t index)
{
    [argument_table setResource:acceleration_structure.gpuResourceID atBufferIndex:index];
}

void SetView(id<MTL4ArgumentTable> argument_table, const std::shared_ptr<MTView>& view, uint32_t index)
{
    switch (view->GetViewDesc().view_type) {
    case ViewType::kConstantBuffer:
    case ViewType::kBuffer:
    case ViewType::kRWBuffer:
    case ViewType::kStructuredBuffer:
    case ViewType::kRWStructuredBuffer:
        SetBuffer(argument_table, view->GetBuffer(), view->GetViewDesc().offset, index);
        break;
    case ViewType::kSampler:
        SetSamplerState(argument_table, view->GetSampler(), index);
        break;
    case ViewType::kTexture:
    case ViewType::kRWTexture:
        SetTexture(argument_table, view->GetTexture(), index);
        break;
    case ViewType::kAccelerationStructure:
        SetAccelerationStructure(argument_table, view->GetAccelerationStructure(), index);
        break;
    default:
        NOTREACHED();
    }
}

void ApplyDirectArguments(const std::shared_ptr<Pipeline>& state,
                          const std::map<ShaderType, id<MTL4ArgumentTable>>& argument_tables,
                          const std::vector<BindKey>& bind_keys,
                          const std::vector<BindingDesc>& bindings,
                          MTDevice& device)
{
    decltype(auto) program = state->As<MTPipeline>().GetProgram();
    for (const auto& binding : bindings) {
        const BindKey& bind_key = binding.bind_key;
        if (bind_key.count == ~0) {
            continue;
        }
        decltype(auto) mt_view = std::static_pointer_cast<MTView>(binding.view);
        decltype(auto) shader = program->GetShader(bind_key.shader_type);
        uint32_t index = shader->As<MTShader>().GetIndex(bind_key);
        SetView(argument_tables.at(bind_key.shader_type), mt_view, index);
    }

    for (const auto& bind_key : bind_keys) {
        if (bind_key.count != ~0) {
            continue;
        }
        decltype(auto) shader = program->GetShader(bind_key.shader_type);
        uint32_t index = shader->As<MTShader>().GetIndex(bind_key);
        auto buffer = device.GetBindlessArgumentBuffer().GetArgumentBuffer();
        SetBuffer(argument_tables.at(bind_key.shader_type), buffer, 0, index);
    }
}

} // namespace

MTBindingSet::MTBindingSet(MTDevice& device, const std::shared_ptr<MTBindingSetLayout>& layout)
    : m_device(device)
{
    m_direct_bind_keys = layout->GetBindKeys();
}

void MTBindingSet::WriteBindings(const std::vector<BindingDesc>& bindings)
{
    m_resources.clear();
    for (const auto& binding : bindings) {
        decltype(auto) bind_key = binding.bind_key;
        if (bind_key.count == ~0) {
            continue;
        }
        decltype(auto) view = std::static_pointer_cast<MTView>(binding.view);
        id<MTLResource> resource = view->GetNativeResource();
        if (!resource) {
            continue;
        }
        m_resources.push_back(resource);
    }

    m_direct_bindings = bindings;
}

void MTBindingSet::Apply(const std::map<ShaderType, id<MTL4ArgumentTable>>& argument_tables,
                         const std::shared_ptr<Pipeline>& state)
{
    ApplyDirectArguments(state, argument_tables, m_direct_bind_keys, m_direct_bindings, m_device);
}

void MTBindingSet::AddResourcesToResidencySet(id<MTLResidencySet> residency_set)
{
    for (const auto& resource : m_resources) {
        [residency_set addAllocation:resource];
    }
}

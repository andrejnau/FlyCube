#include "BindingSet/MTBindingSet.h"

#include "BindingSetLayout/MTBindingSetLayout.h"
#include "Device/MTDevice.h"
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

} // namespace

MTBindingSet::MTBindingSet(MTDevice& device, const std::shared_ptr<MTBindingSetLayout>& layout)
    : m_device(device)
{
    for (const auto& bind_key : layout->GetBindKeys()) {
        if (bind_key.count == kBindlessCount) {
            m_bindless_bind_keys.insert(bind_key);
        }
    }
}

void MTBindingSet::WriteBindings(const std::vector<BindingDesc>& bindings)
{
    for (const auto& [bind_key, view] : bindings) {
        assert(bind_key.count != kBindlessCount);
        m_direct_bindings.insert_or_assign(bind_key, view);
    }
}

void MTBindingSet::Apply(const std::map<ShaderType, id<MTL4ArgumentTable>>& argument_tables,
                         const std::shared_ptr<Pipeline>& state,
                         id<MTLResidencySet> residency_set)
{
    for (const auto& [bind_key, view] : m_direct_bindings) {
        decltype(auto) shader = state->As<MTPipeline>().GetShader(bind_key.shader_type);
        uint32_t index = shader->As<MTShader>().GetIndex(bind_key);
        SetView(argument_tables.at(bind_key.shader_type), std::static_pointer_cast<MTView>(view), index);
        if (view) {
            id<MTLResource> resource = view->As<MTView>().GetNativeResource();
            if (resource) {
                [residency_set addAllocation:resource];
            }
        }
    }

    if (m_bindless_bind_keys.empty()) {
        return;
    }

    id<MTLBuffer> buffer = m_device.GetBindlessArgumentBuffer().GetArgumentBuffer();
    for (const auto& bind_key : m_bindless_bind_keys) {
        decltype(auto) shader = state->As<MTPipeline>().GetShader(bind_key.shader_type);
        uint32_t index = shader->As<MTShader>().GetIndex(bind_key);
        SetBuffer(argument_tables.at(bind_key.shader_type), buffer, 0, index);
    }
    [residency_set addAllocation:buffer];
}

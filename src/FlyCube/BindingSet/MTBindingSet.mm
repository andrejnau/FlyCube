#include "BindingSet/MTBindingSet.h"

#include "BindingSetLayout/MTBindingSetLayout.h"
#include "Device/MTDevice.h"
#include "HLSLCompiler/MSLConverter.h"
#include "Pipeline/MTPipeline.h"
#include "Shader/MTShader.h"
#include "View/MTView.h"

namespace {

uint32_t GetRemappedSlot(BindKey key)
{
    assert(UseArgumentBuffers());
    return key.slot;
}

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
        assert(false);
        break;
    }
}

void ValidateRemappedSlots(const std::shared_ptr<Pipeline>& state, const std::vector<BindKey>& bind_keys)
{
#ifndef NDEBUG
    if (UseArgumentBuffers()) {
        decltype(auto) program = state->As<MTPipeline>().GetProgram();
        for (const auto& bind_key : bind_keys) {
            decltype(auto) shader = program->GetShader(bind_key.shader_type);
            uint32_t index = GetRemappedSlot(bind_key);
            assert(index == shader->As<MTShader>().GetIndex(bind_key));
        }
    }
#endif
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
        if (UseArgumentBuffers()) {
            assert(bind_key.space >= spirv_cross::kMaxArgumentBuffers);
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
    , m_layout(layout)
{
    if (!UseArgumentBuffers()) {
        m_direct_bind_keys = m_layout->GetBindKeys();
        return;
    }

    const std::vector<BindKey>& bind_keys = m_layout->GetBindKeys();
    for (BindKey bind_key : bind_keys) {
        if (bind_key.space >= spirv_cross::kMaxArgumentBuffers || bind_key.count == ~0) {
            m_direct_bind_keys.push_back(bind_key);
            continue;
        }
        auto shader_space = std::make_pair(bind_key.shader_type, bind_key.space);
        m_slots_count[shader_space] = std::max(m_slots_count[shader_space], GetRemappedSlot(bind_key) + bind_key.count);
    }
    for (const auto& [shader_space, slots] : m_slots_count) {
        m_argument_buffers[shader_space] = [m_device.GetDevice() newBufferWithLength:slots * sizeof(uint64_t)
                                                                             options:MTLResourceStorageModeShared];
    }
}

void MTBindingSet::WriteBindings(const std::vector<BindingDesc>& bindings)
{
    if (!UseArgumentBuffers()) {
        m_direct_bindings = bindings;
        return;
    }

    m_direct_bindings.clear();

    for (const auto& binding : bindings) {
        decltype(auto) bind_key = binding.bind_key;
        if (bind_key.space >= spirv_cross::kMaxArgumentBuffers) {
            if (bind_key.count != ~0) {
                m_direct_bindings.push_back(binding);
            }
            continue;
        }
        decltype(auto) view = std::static_pointer_cast<MTView>(binding.view);
        assert(view->GetViewDesc().view_type == bind_key.view_type);

        uint32_t index = GetRemappedSlot(bind_key);
        uint32_t slots = m_slots_count[{ bind_key.shader_type, bind_key.space }];
        assert(index < slots);
        uint64_t* arguments =
            static_cast<uint64_t*>(m_argument_buffers[{ bind_key.shader_type, bind_key.space }].contents);
        arguments[index] = view->GetGpuAddress();
    }
}

void MTBindingSet::Apply(const std::map<ShaderType, id<MTL4ArgumentTable>>& argument_tables,
                         const std::shared_ptr<Pipeline>& state)
{
    ValidateRemappedSlots(state, m_layout->GetBindKeys());
    for (const auto& [key, slots] : m_slots_count) {
        SetBuffer(argument_tables.at(key.first), m_argument_buffers[key], 0, key.second);
    }
    ApplyDirectArguments(state, argument_tables, m_direct_bind_keys, m_direct_bindings, m_device);
}

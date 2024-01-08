#include "BindingSet/MTArgumentBuffer.h"

#include "BindingSet/MTDirectArguments.h"
#include "BindingSetLayout/MTBindingSetLayout.h"
#include "Device/MTDevice.h"
#include "Pipeline/MTPipeline.h"
#include "Shader/MTShader.h"
#include "View/MTView.h"

namespace {

MTLRenderStages GetStage(ShaderType type)
{
    switch (type) {
    case ShaderType::kPixel:
        return MTLRenderStageFragment;
    case ShaderType::kVertex:
        return MTLRenderStageVertex;
    default:
        assert(false);
        return 0;
    }
}

MTLResourceUsage GetUsage(ViewType type)
{
    switch (type) {
    case ViewType::kAccelerationStructure:
    case ViewType::kBuffer:
    case ViewType::kConstantBuffer:
    case ViewType::kSampler:
    case ViewType::kStructuredBuffer:
    case ViewType::kTexture:
        return MTLResourceUsageRead;
    case ViewType::kRWBuffer:
    case ViewType::kRWStructuredBuffer:
    case ViewType::kRWTexture:
        return MTLResourceUsageWrite;
    default:
        assert(false);
        return MTLResourceUsageRead | MTLResourceUsageWrite;
    }
}

void ValidateIRemappedSlots(const std::shared_ptr<Pipeline>& state, const std::vector<BindingDesc>& bindings)
{
#ifndef NDEBUG
    for (const auto& binding : bindings) {
        const BindKey& bind_key = binding.bind_key;
        decltype(auto) program = state->As<MTPipeline>().GetProgram();
        decltype(auto) shader = program->GetShader(bind_key.shader_type);
        uint32_t index = shader->As<MTShader>().GetIndex(bind_key);
        assert(bind_key.GetRemappedSlot() == index);
    }
#endif
}

} // namespace

MTArgumentBuffer::MTArgumentBuffer(MTDevice& device, const std::shared_ptr<MTBindingSetLayout>& layout)
    : m_device(device)
    , m_layout(layout)
{
    const std::vector<BindKey>& bind_keys = m_layout->GetBindKeys();
    for (BindKey bind_key : bind_keys) {
        if (bind_key.space >= spirv_cross::kMaxArgumentBuffers) {
            m_direct_bind_keys.push_back(bind_key);
            continue;
        }
        auto shader_space = std::make_pair(bind_key.shader_type, bind_key.space);
        m_slots_count[shader_space] = std::max(m_slots_count[shader_space], bind_key.GetRemappedSlot() + 1);
    }
    for (const auto& [shader_space, slots] : m_slots_count) {
        m_argument_buffers[shader_space] = [m_device.GetDevice() newBufferWithLength:slots * sizeof(uint64_t)
                                                                             options:MTLResourceStorageModeShared];
    }
}

void MTArgumentBuffer::WriteBindings(const std::vector<BindingDesc>& bindings)
{
    m_bindings = bindings;
    m_direct_bindings.clear();
    m_compure_resouces.clear();
    m_graphics_resouces.clear();

    const std::vector<BindKey>& bind_keys = m_layout->GetBindKeys();
    for (const auto& binding : m_bindings) {
        decltype(auto) bind_key = binding.bind_key;
        if (bind_key.space >= spirv_cross::kMaxArgumentBuffers) {
            m_direct_bindings.push_back(binding);
            continue;
        }
        decltype(auto) view = std::static_pointer_cast<MTView>(binding.view);
        assert(view->GetViewDesc().view_type == bind_key.view_type);

        uint32_t index = bind_key.GetRemappedSlot();
        uint32_t slots = m_slots_count[{ bind_key.shader_type, bind_key.space }];
        assert(index < slots);
        uint64_t* arguments =
            static_cast<uint64_t*>(m_argument_buffers[{ bind_key.shader_type, bind_key.space }].contents);
        arguments[index] = view->GetGpuAddress();

        id<MTLResource> resource = view->GetNativeResource();
        if (!resource) {
            continue;
        }

        MTLResourceUsage usage = GetUsage(bind_key.view_type);
        if (bind_key.shader_type == ShaderType::kCompute) {
            m_compure_resouces[usage].push_back(resource);
        } else {
            m_graphics_resouces[{ GetStage(bind_key.shader_type), usage }].push_back(resource);
        }
    }
}

void MTArgumentBuffer::Apply(id<MTLRenderCommandEncoder> render_encoder, const std::shared_ptr<Pipeline>& state)
{
    ValidateIRemappedSlots(state, m_bindings);
    for (const auto& [key, slots] : m_slots_count) {
        switch (key.first) {
        case ShaderType::kVertex:
            [render_encoder setVertexBuffer:m_argument_buffers[key] offset:0 atIndex:key.second];
            break;
        case ShaderType::kPixel:
            [render_encoder setFragmentBuffer:m_argument_buffers[key] offset:0 atIndex:key.second];
            break;
        default:
            assert(false);
            break;
        }
    }
    for (const auto& [stages_usage, resources] : m_graphics_resouces) {
        [render_encoder useResources:resources.data()
                               count:resources.size()
                               usage:stages_usage.second
                              stages:stages_usage.first];
    }
    MTDirectArguments::ApplyDirectArgs(render_encoder, state, m_direct_bind_keys, m_direct_bindings);
}

void MTArgumentBuffer::Apply(id<MTLComputeCommandEncoder> compute_encoder, const std::shared_ptr<Pipeline>& state)
{
    ValidateIRemappedSlots(state, m_bindings);
    for (const auto& [key, slots] : m_slots_count) {
        switch (key.first) {
        case ShaderType::kCompute:
            [compute_encoder setBuffer:m_argument_buffers[key] offset:0 atIndex:key.second];
            break;
        default:
            assert(false);
            break;
        }
    }
    for (const auto& [usage, resources] : m_compure_resouces) {
        [compute_encoder useResources:resources.data() count:resources.size() usage:usage];
    }
    MTDirectArguments::ApplyDirectArgs(compute_encoder, state, m_direct_bind_keys, m_direct_bindings);
}

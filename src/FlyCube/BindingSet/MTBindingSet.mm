#include "BindingSet/MTBindingSet.h"

#include "BindingSetLayout/MTBindingSetLayout.h"
#include "Device/MTDevice.h"
#include "HLSLCompiler/MSLConverter.h"
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

template <typename CommandEncoderType>
constexpr bool IsComputeEncoder()
{
    return std::is_same_v<CommandEncoderType, id<MTLComputeCommandEncoder>>;
}

template <typename CommandEncoderType>
void SetBuffer(ShaderType shader_type,
               CommandEncoderType encoder,
               id<MTLBuffer> buffer,
               uint32_t offset,
               uint32_t index)
{
    if constexpr (IsComputeEncoder<CommandEncoderType>()) {
        [encoder setBuffer:buffer offset:offset atIndex:index];
    } else if (shader_type == ShaderType::kVertex) {
        [encoder setVertexBuffer:buffer offset:offset atIndex:index];
    } else if (shader_type == ShaderType::kPixel) {
        [encoder setFragmentBuffer:buffer offset:offset atIndex:index];
    }
}

template <typename CommandEncoderType>
void SetSamplerState(ShaderType shader_type, CommandEncoderType encoder, id<MTLSamplerState> sampler, uint32_t index)
{
    if constexpr (IsComputeEncoder<CommandEncoderType>()) {
        [encoder setSamplerState:sampler atIndex:index];
    } else if (shader_type == ShaderType::kVertex) {
        [encoder setVertexSamplerState:sampler atIndex:index];
    } else if (shader_type == ShaderType::kPixel) {
        [encoder setFragmentSamplerState:sampler atIndex:index];
    }
}

template <typename CommandEncoderType>
void SetTexture(ShaderType shader_type, CommandEncoderType encoder, id<MTLTexture> texture, uint32_t index)
{
    if constexpr (IsComputeEncoder<CommandEncoderType>()) {
        [encoder setTexture:texture atIndex:index];
    } else if (shader_type == ShaderType::kVertex) {
        [encoder setVertexTexture:texture atIndex:index];
    } else if (shader_type == ShaderType::kPixel) {
        [encoder setFragmentTexture:texture atIndex:index];
    }
}

template <typename CommandEncoderType>
void SetAccelerationStructure(ShaderType shader_type,
                              CommandEncoderType encoder,
                              id<MTLAccelerationStructure> acceleration_structure,
                              uint32_t index)
{
    if constexpr (IsComputeEncoder<CommandEncoderType>()) {
        [encoder setAccelerationStructure:acceleration_structure atBufferIndex:index];
    } else if (shader_type == ShaderType::kVertex) {
        [encoder setVertexAccelerationStructure:acceleration_structure atBufferIndex:index];
    } else if (shader_type == ShaderType::kPixel) {
        [encoder setFragmentAccelerationStructure:acceleration_structure atBufferIndex:index];
    }
}

template <typename CommandEncoderType>
void SetView(ShaderType shader_type, CommandEncoderType encoder, const std::shared_ptr<MTView>& view, uint32_t index)
{
    switch (view->GetViewDesc().view_type) {
    case ViewType::kConstantBuffer:
    case ViewType::kBuffer:
    case ViewType::kRWBuffer:
    case ViewType::kStructuredBuffer:
    case ViewType::kRWStructuredBuffer:
        SetBuffer(shader_type, encoder, view->GetBuffer(), view->GetViewDesc().offset, index);
        break;
    case ViewType::kSampler:
        SetSamplerState(shader_type, encoder, view->GetSampler(), index);
        break;
    case ViewType::kTexture:
    case ViewType::kRWTexture:
        SetTexture(shader_type, encoder, view->GetTexture(), index);
        break;
    case ViewType::kAccelerationStructure:
        SetAccelerationStructure(shader_type, encoder, view->GetAccelerationStructure(), index);
        break;
    default:
        assert(false);
        break;
    }
}

void ValidateRemappedSlots(const std::shared_ptr<Pipeline>& state, const std::vector<BindKey>& bind_keys)
{
#ifndef NDEBUG
    decltype(auto) program = state->As<MTPipeline>().GetProgram();
    for (const auto& bind_key : bind_keys) {
        decltype(auto) shader = program->GetShader(bind_key.shader_type);
        uint32_t index = bind_key.GetRemappedSlot();
        assert(index == shader->As<MTShader>().GetIndex(bind_key));
    }
#endif
}

template <typename CommandEncoderType>
void ApplyDirectArguments(CommandEncoderType encoder,
                          const std::vector<BindKey>& bind_keys,
                          const std::vector<BindingDesc>& bindings,
                          MTDevice& device)
{
    for (const auto& binding : bindings) {
        const BindKey& bind_key = binding.bind_key;
        if (bind_key.count == ~0) {
            continue;
        }
        decltype(auto) mt_view = std::static_pointer_cast<MTView>(binding.view);
        uint32_t index = bind_key.GetRemappedSlot();
        SetView(bind_key.shader_type, encoder, mt_view, index);
    }

    bool has_bindless = false;
    for (const auto& bind_key : bind_keys) {
        if (bind_key.space < spirv_cross::kMaxArgumentBuffers || bind_key.count != ~0) {
            continue;
        }
        has_bindless = true;
        uint32_t index = bind_key.GetRemappedSlot();
        auto buffer = device.GetBindlessArgumentBuffer().GetArgumentBuffer();
        SetBuffer(bind_key.shader_type, encoder, buffer, 0, index);
    }

    if (has_bindless) {
        decltype(auto) resources_usage = device.GetBindlessArgumentBuffer().GetResourcesUsage();
        for (const auto& resource_usage : resources_usage) {
            if (!resource_usage.first) {
                continue;
            }
            if constexpr (IsComputeEncoder<CommandEncoderType>()) {
                [encoder useResource:resource_usage.first usage:resource_usage.second];
            } else {
                [encoder useResource:resource_usage.first
                               usage:resource_usage.second
                              stages:MTLRenderStageVertex | MTLRenderStageFragment];
            }
        }
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
        m_slots_count[shader_space] =
            std::max(m_slots_count[shader_space], bind_key.GetRemappedSlot() + bind_key.count);
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
    m_compure_resouces.clear();
    m_graphics_resouces.clear();

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

        MTLResourceUsage usage = view->GetUsage();
        if (bind_key.shader_type == ShaderType::kCompute) {
            m_compure_resouces[usage].push_back(resource);
        } else {
            m_graphics_resouces[{ GetStage(bind_key.shader_type), usage }].push_back(resource);
        }
    }
}

void MTBindingSet::Apply(id<MTLRenderCommandEncoder> render_encoder, const std::shared_ptr<Pipeline>& state)
{
    ValidateRemappedSlots(state, m_layout->GetBindKeys());
    for (const auto& [key, slots] : m_slots_count) {
        SetBuffer(key.first, render_encoder, m_argument_buffers[key], 0, key.second);
    }
    for (const auto& [stages_usage, resources] : m_graphics_resouces) {
        [render_encoder useResources:resources.data()
                               count:resources.size()
                               usage:stages_usage.second
                              stages:stages_usage.first];
    }
    ApplyDirectArguments(render_encoder, m_direct_bind_keys, m_direct_bindings, m_device);
}

void MTBindingSet::Apply(id<MTLComputeCommandEncoder> compute_encoder, const std::shared_ptr<Pipeline>& state)
{
    ValidateRemappedSlots(state, m_layout->GetBindKeys());
    for (const auto& [key, slots] : m_slots_count) {
        SetBuffer(key.first, compute_encoder, m_argument_buffers[key], 0, key.second);
    }
    for (const auto& [usage, resources] : m_compure_resouces) {
        [compute_encoder useResources:resources.data() count:resources.size() usage:usage];
    }
    ApplyDirectArguments(compute_encoder, m_direct_bind_keys, m_direct_bindings, m_device);
}

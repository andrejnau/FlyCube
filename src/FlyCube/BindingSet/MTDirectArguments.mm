#include "BindingSet/MTDirectArguments.h"

#include "BindingSetLayout/MTBindingSetLayout.h"
#include "Device/MTDevice.h"
#include "Pipeline/MTPipeline.h"
#include "Shader/MTShader.h"
#include "View/MTView.h"

namespace {

template <typename CommandEncoderType>
constexpr bool is_compute_encoder()
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
    if constexpr (is_compute_encoder<CommandEncoderType>()) {
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
    if constexpr (is_compute_encoder<CommandEncoderType>()) {
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
    if constexpr (is_compute_encoder<CommandEncoderType>()) {
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
    if constexpr (is_compute_encoder<CommandEncoderType>()) {
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
    case ViewType::kRWStructuredBuffer: {
        SetBuffer(shader_type, encoder, view->GetBuffer(), view->GetViewDesc().offset, index);
        break;
    }
    case ViewType::kSampler: {
        SetSamplerState(shader_type, encoder, view->GetSampler(), index);
        break;
    }
    case ViewType::kTexture:
    case ViewType::kRWTexture: {
        SetTexture(shader_type, encoder, view->GetTexture(), index);
        break;
    }
    case ViewType::kAccelerationStructure: {
        SetAccelerationStructure(shader_type, encoder, view->GetAccelerationStructure(), index);
        break;
    }
    default:
        assert(false);
        break;
    }
}

} // namespace

MTDirectArguments::MTDirectArguments(MTDevice& device, const std::shared_ptr<MTBindingSetLayout>& layout)
    : m_device(device)
    , m_layout(layout)
{
}

void MTDirectArguments::WriteBindings(const std::vector<BindingDesc>& bindings)
{
    m_bindings = bindings;
}

template <typename CommandEncoderType>
void MTDirectArguments::ApplyDirectArgs(CommandEncoderType encoder,
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
            if constexpr (is_compute_encoder<CommandEncoderType>()) {
                [encoder useResource:resource_usage.first usage:resource_usage.second];
            } else {
                [encoder useResource:resource_usage.first
                               usage:resource_usage.second
                              stages:MTLRenderStageVertex | MTLRenderStageFragment];
            }
        }
    }
}

void MTDirectArguments::ValidateRemappedSlots(const std::shared_ptr<Pipeline>& state,
                                              const std::vector<BindKey>& bind_keys)
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

void MTDirectArguments::Apply(id<MTLRenderCommandEncoder> render_encoder, const std::shared_ptr<Pipeline>& state)
{
    ValidateRemappedSlots(state, m_layout->GetBindKeys());
    ApplyDirectArgs(render_encoder, m_layout->GetBindKeys(), m_bindings, m_device);
}

void MTDirectArguments::Apply(id<MTLComputeCommandEncoder> compute_encoder, const std::shared_ptr<Pipeline>& state)
{
    ValidateRemappedSlots(state, m_layout->GetBindKeys());
    ApplyDirectArgs(compute_encoder, m_layout->GetBindKeys(), m_bindings, m_device);
}

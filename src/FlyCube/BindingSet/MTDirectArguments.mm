#include "BindingSet/MTDirectArguments.h"

#include "BindingSetLayout/MTBindingSetLayout.h"
#include "Device/MTDevice.h"
#include "Pipeline/MTPipeline.h"
#include "Shader/MTShader.h"
#include "View/MTView.h"

namespace {

template <ShaderType shader_type, typename CommandEncoderType>
constexpr bool is_vertex()
{
    return shader_type == ShaderType::kVertex && std::is_same_v<CommandEncoderType, id<MTLRenderCommandEncoder>>;
}

template <ShaderType shader_type, typename CommandEncoderType>
constexpr bool is_pixel()
{
    return shader_type == ShaderType::kPixel && std::is_same_v<CommandEncoderType, id<MTLRenderCommandEncoder>>;
}

template <ShaderType shader_type, typename CommandEncoderType>
constexpr bool is_compute()
{
    return shader_type == ShaderType::kCompute && std::is_same_v<CommandEncoderType, id<MTLComputeCommandEncoder>>;
}

template <ShaderType shader_type, typename CommandEncoderType>
void SetBuffer(CommandEncoderType encoder, id<MTLBuffer> buffer, uint32_t offset, uint32_t index)
{
    if constexpr (is_vertex<shader_type, CommandEncoderType>()) {
        [encoder setVertexBuffer:buffer offset:offset atIndex:index];
    } else if constexpr (is_pixel<shader_type, CommandEncoderType>()) {
        [encoder setFragmentBuffer:buffer offset:offset atIndex:index];
    } else if constexpr (is_compute<shader_type, CommandEncoderType>()) {
        [encoder setBuffer:buffer offset:offset atIndex:index];
    }
}

template <ShaderType shader_type, typename CommandEncoderType>
void SetSamplerState(CommandEncoderType encoder, id<MTLSamplerState> sampler, uint32_t index)
{
    if constexpr (is_vertex<shader_type, CommandEncoderType>()) {
        [encoder setVertexSamplerState:sampler atIndex:index];
    } else if constexpr (is_pixel<shader_type, CommandEncoderType>()) {
        [encoder setFragmentSamplerState:sampler atIndex:index];
    } else if constexpr (is_compute<shader_type, CommandEncoderType>()) {
        [encoder setSamplerState:sampler atIndex:index];
    }
}

template <ShaderType shader_type, typename CommandEncoderType>
void SetTexture(CommandEncoderType encoder, id<MTLTexture> texture, uint32_t index)
{
    if constexpr (is_vertex<shader_type, CommandEncoderType>()) {
        [encoder setVertexTexture:texture atIndex:index];
    } else if constexpr (is_pixel<shader_type, CommandEncoderType>()) {
        [encoder setFragmentTexture:texture atIndex:index];
    } else if constexpr (is_compute<shader_type, CommandEncoderType>()) {
        [encoder setTexture:texture atIndex:index];
    }
}

template <ShaderType shader_type, typename CommandEncoderType>
void SetAccelerationStructure(CommandEncoderType encoder,
                              id<MTLAccelerationStructure> acceleration_structure,
                              uint32_t index)
{
    if constexpr (is_vertex<shader_type, CommandEncoderType>()) {
        [encoder setVertexAccelerationStructure:acceleration_structure atBufferIndex:index];
    } else if constexpr (is_pixel<shader_type, CommandEncoderType>()) {
        [encoder setFragmentAccelerationStructure:acceleration_structure atBufferIndex:index];
    } else if constexpr (is_compute<shader_type, CommandEncoderType>()) {
        [encoder setAccelerationStructure:acceleration_structure atBufferIndex:index];
    }
}

template <ShaderType shader_type, typename CommandEncoderType>
void SetView(CommandEncoderType encoder, ViewType view_type, const std::shared_ptr<MTView>& view, uint32_t index)
{
    decltype(auto) mt_resource = view->GetMTResource();
    switch (view_type) {
    case ViewType::kConstantBuffer:
    case ViewType::kBuffer:
    case ViewType::kRWBuffer:
    case ViewType::kStructuredBuffer:
    case ViewType::kRWStructuredBuffer: {
        id<MTLBuffer> buffer = {};
        if (mt_resource) {
            buffer = mt_resource->buffer.res;
        }
        SetBuffer<shader_type>(encoder, buffer, view->GetViewDesc().offset, index);
        break;
    }
    case ViewType::kSampler: {
        id<MTLSamplerState> sampler = {};
        if (mt_resource) {
            sampler = mt_resource->sampler.res;
        }
        SetSamplerState<shader_type>(encoder, sampler, index);
        break;
    }
    case ViewType::kTexture:
    case ViewType::kRWTexture: {
        id<MTLTexture> texture = view->GetTextureView();
        if (!texture && mt_resource) {
            texture = mt_resource->texture.res;
        }
        SetTexture<shader_type>(encoder, texture, index);
        break;
    }
    case ViewType::kAccelerationStructure: {
        id<MTLAccelerationStructure> acceleration_structure = {};
        if (mt_resource) {
            acceleration_structure = mt_resource->acceleration_structure;
        }
        SetAccelerationStructure<shader_type>(encoder, acceleration_structure, index);
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
                                        const std::shared_ptr<Pipeline>& state,
                                        const std::vector<BindKey>& bind_keys,
                                        const std::vector<BindingDesc>& bindings,
                                        MTDevice& device)
{
    for (const auto& binding : bindings) {
        const BindKey& bind_key = binding.bind_key;
        decltype(auto) mt_view = std::static_pointer_cast<MTView>(binding.view);
        decltype(auto) program = state->As<MTPipeline>().GetProgram();
        decltype(auto) shader = program->GetShader(bind_key.shader_type);
        uint32_t index = bind_key.GetRemappedSlot();
        assert(index == shader->As<MTShader>().GetIndex(bind_key));

        switch (bind_key.shader_type) {
        case ShaderType::kVertex:
            SetView<ShaderType::kVertex>(encoder, bind_key.view_type, mt_view, index);
            break;
        case ShaderType::kPixel:
            SetView<ShaderType::kPixel>(encoder, bind_key.view_type, mt_view, index);
            break;
        case ShaderType::kCompute:
            SetView<ShaderType::kCompute>(encoder, bind_key.view_type, mt_view, index);
            break;
        default:
            assert(false);
            break;
        }
    }
    bool has_bindless = false;
    for (const auto& bind_key : bind_keys) {
        if (bind_key.space < spirv_cross::kMaxArgumentBuffers || bind_key.count != ~0) {
            continue;
        }
        has_bindless = true;
        decltype(auto) program = state->As<MTPipeline>().GetProgram();
        decltype(auto) shader = program->GetShader(bind_key.shader_type);
        uint32_t index = bind_key.GetRemappedSlot();
        assert(index == shader->As<MTShader>().GetIndex(bind_key));

        auto buffer = device.GetBindlessArgumentBuffer().GetArgumentBuffer();
        switch (bind_key.shader_type) {
        case ShaderType::kVertex:
            SetBuffer<ShaderType::kVertex>(encoder, buffer, 0, index);
            break;
        case ShaderType::kPixel:
            SetBuffer<ShaderType::kPixel>(encoder, buffer, 0, index);
            break;
        case ShaderType::kCompute:
            SetBuffer<ShaderType::kCompute>(encoder, buffer, 0, index);
            break;
        default:
            assert(false);
            break;
        }
    }
    if (has_bindless) {
        decltype(auto) resources_usage = device.GetBindlessArgumentBuffer().GetResourcesUsage();
        for (const auto& resource_usage : resources_usage) {
            if (!resource_usage.first) {
                continue;
            }
            if constexpr (std::is_same_v<CommandEncoderType, id<MTLRenderCommandEncoder>>) {
                [encoder useResource:resource_usage.first
                               usage:resource_usage.second
                              stages:MTLRenderStageVertex | MTLRenderStageFragment];
            } else if constexpr (std::is_same_v<CommandEncoderType, id<MTLComputeCommandEncoder>>) {
                [encoder useResource:resource_usage.first usage:resource_usage.second];
            }
        }
    }
}

void MTDirectArguments::Apply(id<MTLRenderCommandEncoder> render_encoder, const std::shared_ptr<Pipeline>& state)
{
    ApplyDirectArgs(render_encoder, state, m_layout->GetBindKeys(), m_bindings, m_device);
}

void MTDirectArguments::Apply(id<MTLComputeCommandEncoder> compute_encoder, const std::shared_ptr<Pipeline>& state)
{
    ApplyDirectArgs(compute_encoder, state, m_layout->GetBindKeys(), m_bindings, m_device);
}

#include "BindingSet/MTBindingSet.h"

#include "BindingSetLayout/MTBindingSetLayout.h"
#include "Device/MTDevice.h"
#include "Pipeline/MTGraphicsPipeline.h"
#include "Shader/MTShader.h"
#include "View/MTView.h"

MTBindingSet::MTBindingSet(MTDevice& device, const std::shared_ptr<MTBindingSetLayout>& layout)
    : m_device(device)
    , m_layout(layout)
{
}

void MTBindingSet::WriteBindings(const std::vector<BindingDesc>& bindings)
{
    m_bindings = bindings;
}

template <ShaderType shader_type, typename CommandEncoderType>
static void SetBuffer(CommandEncoderType encoder, id<MTLBuffer> buffer, uint32_t offset, uint32_t index)
{
    if constexpr (shader_type == ShaderType::kVertex) {
        [encoder setVertexBuffer:buffer offset:offset atIndex:index];
    } else if constexpr (shader_type == ShaderType::kPixel) {
        [encoder setFragmentBuffer:buffer offset:offset atIndex:index];
    } else if constexpr (shader_type == ShaderType::kCompute) {
        [encoder setBuffer:buffer offset:offset atIndex:index];
    }
}

template <ShaderType shader_type, typename CommandEncoderType>
static void SetSamplerState(CommandEncoderType encoder, id<MTLSamplerState> sampler, uint32_t index)
{
    if constexpr (shader_type == ShaderType::kVertex) {
        [encoder setVertexSamplerState:sampler atIndex:index];
    } else if constexpr (shader_type == ShaderType::kPixel) {
        [encoder setFragmentSamplerState:sampler atIndex:index];
    } else if constexpr (shader_type == ShaderType::kCompute) {
        [encoder setSamplerState:sampler atIndex:index];
    }
}

template <ShaderType shader_type, typename CommandEncoderType>
static void SetTexture(CommandEncoderType encoder, id<MTLTexture> texture, uint32_t index)
{
    if constexpr (shader_type == ShaderType::kVertex) {
        [encoder setVertexTexture:texture atIndex:index];
    } else if constexpr (shader_type == ShaderType::kPixel) {
        [encoder setFragmentTexture:texture atIndex:index];
    } else if constexpr (shader_type == ShaderType::kCompute) {
        [encoder setTexture:texture atIndex:index];
    }
}

template <ShaderType shader_type, typename CommandEncoderType>
static void SetAccelerationStructure(CommandEncoderType encoder,
                                     id<MTLAccelerationStructure> acceleration_structure,
                                     uint32_t index)
{
    if constexpr (shader_type == ShaderType::kVertex) {
        [encoder setVertexAccelerationStructure:acceleration_structure atBufferIndex:index];
    } else if constexpr (shader_type == ShaderType::kPixel) {
        [encoder setFragmentAccelerationStructure:acceleration_structure atBufferIndex:index];
    } else if constexpr (shader_type == ShaderType::kCompute) {
        [encoder setAccelerationStructure:acceleration_structure atBufferIndex:index];
    }
}

template <ShaderType shader_type, typename CommandEncoderType>
static void SetView(CommandEncoderType encoder, ViewType view_type, const std::shared_ptr<MTView>& view, uint32_t index)
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

template <typename CommandEncoderType>
static void ApplyImpl(CommandEncoderType encoder,
                      const std::shared_ptr<Pipeline>& state,
                      const std::shared_ptr<MTBindingSetLayout>& layout,
                      const std::vector<BindingDesc>& bindings)
{
    const std::vector<BindKey>& bind_keys = layout->GetBindKeys();
    for (const auto& binding : bindings) {
        const BindKey& bind_key = binding.bind_key;
        decltype(auto) mt_view = std::static_pointer_cast<MTView>(binding.view);
        decltype(auto) program = state->As<MTPipeline>().GetProgram();
        decltype(auto) shader = program->GetShader(bind_key.shader_type);
        uint32_t index = shader->As<MTShader>().GetIndex(bind_key);

        if constexpr (std::is_same_v<CommandEncoderType, id<MTLRenderCommandEncoder>>) {
            switch (bind_key.shader_type) {
            case ShaderType::kVertex:
                SetView<ShaderType::kVertex>(encoder, bind_key.view_type, mt_view, index);
                break;
            case ShaderType::kPixel:
                SetView<ShaderType::kPixel>(encoder, bind_key.view_type, mt_view, index);
                break;
            default:
                assert(false);
                break;
            }
        } else if constexpr (std::is_same_v<CommandEncoderType, id<MTLComputeCommandEncoder>>) {
            switch (bind_key.shader_type) {
            case ShaderType::kCompute:
                SetView<ShaderType::kCompute>(encoder, bind_key.view_type, mt_view, index);
                break;
            default:
                assert(false);
                break;
            }
        }
    }
}

void MTBindingSet::Apply(id<MTLRenderCommandEncoder> render_encoder, const std::shared_ptr<Pipeline>& state)
{
    ApplyImpl(render_encoder, state, m_layout, m_bindings);
}

void MTBindingSet::Apply(id<MTLComputeCommandEncoder> compute_encoder, const std::shared_ptr<Pipeline>& state)
{
    ApplyImpl(compute_encoder, state, m_layout, m_bindings);
}

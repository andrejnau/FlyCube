#include "BindingSet/MTBindingSet.h"
#include <Device/MTDevice.h>
#include <BindingSetLayout/MTBindingSetLayout.h>
#include <Pipeline/MTGraphicsPipeline.h>
#include <View/MTView.h>
#include <Shader/MTShader.h>

MTBindingSet::MTBindingSet(MTDevice& device, const std::shared_ptr<MTBindingSetLayout>& layout)
    : m_device(device)
    , m_layout(layout)
{
}

void MTBindingSet::WriteBindings(const std::vector<BindingDesc>& bindings)
{
    m_bindings = bindings;
}

template<typename CommandEncoderType>
static void SetBuffer(CommandEncoderType encoder, ShaderType shader_type, id<MTLBuffer> buffer, uint32_t offset, uint32_t index)
{
    switch (shader_type)
    {
    case ShaderType::kVertex:
        [encoder setVertexBuffer:buffer
                          offset:offset
                         atIndex:index];
        break;
    case ShaderType::kPixel:
        [encoder setFragmentBuffer:buffer
                            offset:offset
                           atIndex:index];
        break;
    case ShaderType::kCompute:
        [encoder setBuffer:buffer
                    offset:offset
                   atIndex:index];
        break;
    default:
        assert(false);
        break;
    }
}

template<typename CommandEncoderType>
static void SetSamplerState(CommandEncoderType encoder, ShaderType shader_type, id<MTLSamplerState> sampler, uint32_t index)
{
    switch (shader_type)
    {
    case ShaderType::kVertex:
        [encoder setVertexSamplerState:sampler
                               atIndex:index];
        break;
    case ShaderType::kPixel:
        [encoder setFragmentSamplerState:sampler
                                 atIndex:index];
        break;
    case ShaderType::kCompute:
        [encoder setSamplerState:sampler
                         atIndex:index];
        break;
    default:
        assert(false);
        break;
    }
}

template<typename CommandEncoderType>
static void SetTexture(CommandEncoderType encoder, ShaderType shader_type, id<MTLTexture> texture, uint32_t index)
{
    switch (shader_type)
    {
    case ShaderType::kVertex:
        [encoder setVertexTexture:texture
                          atIndex:index];
        break;
    case ShaderType::kPixel:
        [encoder setFragmentTexture:texture
                            atIndex:index];
        break;
    case ShaderType::kCompute:
        [encoder setTexture:texture
                    atIndex:index];
        break;
    default:
        assert(false);
        break;
    }
}

template<typename CommandEncoderType>
static void SetView(CommandEncoderType encoder, ShaderType shader_type, ViewType view_type, const std::shared_ptr<MTView>& view, uint32_t index)
{
    decltype(auto) mt_resource = view->GetMTResource();
    switch (view_type)
    {
    case ViewType::kConstantBuffer:
    case ViewType::kBuffer:
    case ViewType::kRWBuffer:
    case ViewType::kStructuredBuffer:
    case ViewType::kRWStructuredBuffer:
    {
        id<MTLBuffer> buffer = {};
        if (mt_resource)
            buffer = mt_resource->buffer.res;
        SetBuffer(encoder, shader_type, buffer, view->GetViewDesc().offset, index);
        break;
    }
    case ViewType::kSampler:
    {
        id<MTLSamplerState> sampler = {};
        if (mt_resource)
            sampler = mt_resource->sampler.res;
        SetSamplerState(encoder, shader_type, sampler, index);
        break;
    }
    case ViewType::kTexture:
    case ViewType::kRWTexture:
    {
        id<MTLTexture> texture = view->GetTextureView();
        if (!texture && mt_resource)
            texture = mt_resource->texture.res;
        SetTexture(encoder, shader_type, texture, index);
        break;
    }
    default:
        assert(false);
        break;
    }
}

template<typename CommandEncoderType>
static void ApplyImpl(CommandEncoderType encoder, const std::shared_ptr<Pipeline>& state, const std::shared_ptr<MTBindingSetLayout>& layout, const std::vector<BindingDesc>& bindings)
{
    const std::vector<BindKey>& bind_keys = layout->GetBindKeys();
    for (const auto& binding : bindings)
    {
        const BindKey& bind_key = binding.bind_key;
        decltype(auto) mt_view = std::static_pointer_cast<MTView>(binding.view);
        decltype(auto) program = state->As<MTPipeline>().GetProgram();
        decltype(auto) shader = program->GetShader(bind_key.shader_type);
        uint32_t index = shader->As<MTShader>().GetIndex(bind_key);
        SetView(encoder, bind_key.shader_type, bind_key.view_type, mt_view, index);
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

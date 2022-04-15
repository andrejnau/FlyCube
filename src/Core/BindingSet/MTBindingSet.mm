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

void MTBindingSet::SetPixelShaderView(id<MTLRenderCommandEncoder> render_encoder, ViewType view_type, const std::shared_ptr<MTView>& view, uint32_t index)
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
        [render_encoder setFragmentBuffer:buffer
                                   offset:0
                                  atIndex:index];
        break;
    }
    case ViewType::kSampler:
    {
        id<MTLSamplerState> sampler = {};
        if (mt_resource)
            sampler = mt_resource->sampler.res;
        [render_encoder setFragmentSamplerState:sampler
                                        atIndex:index];
        break;
    }
    case ViewType::kTexture:
    case ViewType::kRWTexture:
    {
        id<MTLTexture> texture = {};
        if (mt_resource)
            texture = mt_resource->texture.res;
        [render_encoder setFragmentTexture:texture
                                   atIndex:index];
        break;
    }
    case ViewType::kAccelerationStructure:
        assert(false);
        break;
    default:
        assert(false);
        break;
    }
}

void MTBindingSet::SetVertexShaderView(id<MTLRenderCommandEncoder> render_encoder, ViewType view_type, const std::shared_ptr<MTView>& view, uint32_t index)
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
        [render_encoder setVertexBuffer:buffer
                                 offset:0
                                atIndex:index];
        break;
    }
    case ViewType::kSampler:
    {
        id<MTLSamplerState> sampler = {};
        if (mt_resource)
            sampler = mt_resource->sampler.res;
        [render_encoder setVertexSamplerState:sampler
                                      atIndex:index];
        break;
    }
    case ViewType::kTexture:
    case ViewType::kRWTexture:
    {
        id<MTLTexture> texture = {};
        if (mt_resource)
            texture = mt_resource->texture.res;
        [render_encoder setVertexTexture:texture
                                 atIndex:index];
        break;
    }
    case ViewType::kAccelerationStructure:
        assert(false);
        break;
    default:
        assert(false);
        break;
    }
}

void MTBindingSet::SetComputeShaderView(id<MTLComputeCommandEncoder> compute_encoder, ViewType view_type, const std::shared_ptr<MTView>& view, uint32_t index)
{
    decltype(auto) mt_resource = view->GetMTResource();
    switch (view_type)
    {
    case ViewType::kConstantBuffer:
    {
        id<MTLBuffer> buffer = {};
        if (mt_resource)
            buffer = mt_resource->buffer.res;
        [compute_encoder setBuffer:buffer
                            offset:0
                           atIndex:index];
        break;
    }
    case ViewType::kSampler:
    {
        id<MTLSamplerState> sampler = {};
        if (mt_resource)
            sampler = mt_resource->sampler.res;
        [compute_encoder setSamplerState:sampler
                                 atIndex:index];
        break;
    }
    case ViewType::kTexture:
    case ViewType::kRWTexture:
    {
        id<MTLTexture> texture = {};
        if (mt_resource)
            texture = mt_resource->texture.res;
        [compute_encoder setTexture:texture
                            atIndex:index];
        break;
    }
    case ViewType::kBuffer:
    case ViewType::kRWBuffer:
    case ViewType::kStructuredBuffer:
    case ViewType::kRWStructuredBuffer:
    {
        id<MTLBuffer> buffer = {};
        if (mt_resource)
            buffer = mt_resource->buffer.res;
        [compute_encoder setBuffer:buffer
                            offset:0
                           atIndex:index];
        break;
    }
    case ViewType::kAccelerationStructure:
        assert(false);
        break;
    default:
        assert(false);
        break;
    }
}

void MTBindingSet::Apply(id<MTLRenderCommandEncoder> render_encoder, const std::shared_ptr<Pipeline>& state)
{
    const std::vector<BindKey>& bind_keys = m_layout->GetBindKeys();
    for (const auto& binding : m_bindings)
    {
        const BindKey& bind_key = binding.bind_key;
        decltype(auto) mt_view = std::static_pointer_cast<MTView>(binding.view);
        decltype(auto) program = state->As<MTPipeline>().GetProgram();
        decltype(auto) shader = program->GetShader(bind_key.shader_type);
        uint32_t index = shader->As<MTShader>().GetIndex(bind_key);
        switch (bind_key.shader_type)
        {
        case ShaderType::kPixel:
            SetPixelShaderView(render_encoder, bind_key.view_type, mt_view, index);
            break;
        case ShaderType::kVertex:
            SetVertexShaderView(render_encoder, bind_key.view_type, mt_view, index);
            break;
        default:
            assert(false);
            break;
        }
    }
}

void MTBindingSet::Apply(id<MTLComputeCommandEncoder> compute_encoder, const std::shared_ptr<Pipeline>& state)
{
    const std::vector<BindKey>& bind_keys = m_layout->GetBindKeys();
    for (const auto& binding : m_bindings)
    {
        const BindKey& bind_key = binding.bind_key;
        decltype(auto) mt_view = std::static_pointer_cast<MTView>(binding.view);
        decltype(auto) program = state->As<MTPipeline>().GetProgram();
        decltype(auto) shader = program->GetShader(bind_key.shader_type);
        uint32_t index = shader->As<MTShader>().GetIndex(bind_key);
        switch (bind_key.shader_type)
        {
        case ShaderType::kCompute:
            SetComputeShaderView(compute_encoder, bind_key.view_type, mt_view, index);
            break;
        default:
            assert(false);
            break;
        }
    }
}

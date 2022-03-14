#include "BindingSet/MTBindingSet.h"
#include <Device/MTDevice.h>
#include <BindingSetLayout/MTBindingSetLayout.h>
#include <Pipeline/MTGraphicsPipeline.h>
#include <View/MTView.h>

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
        assert(false);
        break;
    case ViewType::kTexture:
    {
        id<MTLTexture> texture = {};
        if (mt_resource)
            texture = mt_resource->texture.res;
        [render_encoder setFragmentTexture:texture
                                   atIndex:index];
        break;
    }
    case ViewType::kRWTexture:
        assert(false);
        break;
    case ViewType::kBuffer:
        assert(false);
        break;
    case ViewType::kRWBuffer:
        assert(false);
        break;
    case ViewType::kStructuredBuffer:
        assert(false);
        break;
    case ViewType::kRWStructuredBuffer:
        assert(false);
        break;
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
        assert(false);
        break;
    case ViewType::kTexture:
        assert(false);
        break;
    case ViewType::kRWTexture:
        assert(false);
        break;
    case ViewType::kBuffer:
        assert(false);
        break;
    case ViewType::kRWBuffer:
        assert(false);
        break;
    case ViewType::kStructuredBuffer:
        assert(false);
        break;
    case ViewType::kRWStructuredBuffer:
        assert(false);
        break;
    case ViewType::kAccelerationStructure:
        assert(false);
        break;
    default:
        assert(false);
        break;
    }
}

void MTBindingSet::Apply(id<MTLRenderCommandEncoder> render_encoder, const std::shared_ptr<MTGraphicsPipeline>& state)
{
    const std::vector<BindKey>& bind_keys = m_layout->GetBindKeys();
    for (const auto& binding : m_bindings)
    {
        const BindKey& bind_key = binding.bind_key;
        decltype(auto) mt_view = std::static_pointer_cast<MTView>(binding.view);
        // TODO: get index
        // decltype(auto) desc = state->GetDesc().program;
        uint32_t index = 0;
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

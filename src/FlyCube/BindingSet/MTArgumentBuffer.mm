#include "BindingSet/MTArgumentBuffer.h"

#include "BindingSetLayout/MTBindingSetLayout.h"
#include "Device/MTDevice.h"
#include "View/MTView.h"

MTArgumentBuffer::MTArgumentBuffer(MTDevice& device, const std::shared_ptr<MTBindingSetLayout>& layout)
    : m_device(device)
    , m_layout(layout)
{
    const std::vector<BindKey>& bind_keys = m_layout->GetBindKeys();
    for (BindKey bind_key : bind_keys) {
        auto shader_space = std::make_pair(bind_key.shader_type, bind_key.space);
        m_slots_count[shader_space] = std::max(m_slots_count[shader_space], bind_key.slot + 1);
    }
    for (const auto& [shader_space, slots] : m_slots_count) {
        m_argument_buffers[shader_space] = [m_device.GetDevice() newBufferWithLength:slots * sizeof(uint64_t)
                                                                             options:MTLResourceStorageModeShared];
    }
}

void MTArgumentBuffer::WriteBindings(const std::vector<BindingDesc>& bindings)
{
    m_bindings = bindings;

    const std::vector<BindKey>& bind_keys = m_layout->GetBindKeys();
    for (const auto& binding : m_bindings) {
        const BindKey& bind_key = binding.bind_key;
        decltype(auto) view = std::static_pointer_cast<MTView>(binding.view);
        decltype(auto) mt_resource = view->GetMTResource();
        uint32_t index = bind_key.slot;
        uint32_t slots = m_slots_count[{ bind_key.shader_type, bind_key.space }];
        assert(index < slots);
        uint64_t* arguments = (uint64_t*)m_argument_buffers[{ bind_key.shader_type, bind_key.space }].contents;

        switch (bind_key.view_type) {
        case ViewType::kConstantBuffer:
        case ViewType::kBuffer:
        case ViewType::kRWBuffer:
        case ViewType::kStructuredBuffer:
        case ViewType::kRWStructuredBuffer: {
            id<MTLBuffer> buffer = {};
            if (mt_resource) {
                buffer = mt_resource->buffer.res;
            }
            arguments[index] = [buffer gpuAddress] + view->GetViewDesc().offset;
            m_resouces.push_back(buffer);
            break;
        }
        case ViewType::kSampler: {
            id<MTLSamplerState> sampler = {};
            if (mt_resource) {
                sampler = mt_resource->sampler.res;
            }
            auto res_id = [sampler gpuResourceID];
            arguments[index] = res_id._impl;
            break;
        }
        case ViewType::kTexture:
        case ViewType::kRWTexture: {
            id<MTLTexture> texture = view->GetTextureView();
            if (!texture && mt_resource) {
                texture = mt_resource->texture.res;
            }
            auto res_id = [texture gpuResourceID];
            arguments[index] = res_id._impl;
            m_resouces.push_back(texture);
            break;
        }
        case ViewType::kAccelerationStructure: {
            id<MTLAccelerationStructure> acceleration_structure = {};
            if (mt_resource) {
                acceleration_structure = mt_resource->acceleration_structure;
            }
            auto res_id = [acceleration_structure gpuResourceID];
            arguments[index] = res_id._impl;
            m_resouces.push_back(acceleration_structure);
            break;
        }
        default:
            assert(false);
            break;
        }
    }
}

void MTArgumentBuffer::Apply(id<MTLRenderCommandEncoder> render_encoder, const std::shared_ptr<Pipeline>& state)
{
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
    [render_encoder useResources:m_resouces.data()
                           count:m_resouces.size()
                           usage:MTLResourceUsageRead | MTLResourceUsageWrite
                          stages:MTLRenderStageVertex | MTLRenderStageFragment];
}

void MTArgumentBuffer::Apply(id<MTLComputeCommandEncoder> compute_encoder, const std::shared_ptr<Pipeline>& state)
{
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
    [compute_encoder useResources:m_resouces.data()
                            count:m_resouces.size()
                            usage:MTLResourceUsageRead | MTLResourceUsageWrite];
}

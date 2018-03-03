#include "Context/DescriptorPool.h"
#include "Context/DX12Context.h"
#include <d3dx12.h>

DescriptorPoolByType::DescriptorPoolByType(DX12Context& context, D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_context(context)
    , m_type(type)
    , m_tail(0)
    , m_size(0)
{
}

bool DescriptorPoolByType::HasDescriptor(BindingInfo info, Resource::Ptr res)
{
    auto it = m_descriptor_offset.find({ info, res });
    if (it != m_descriptor_offset.end())
        return true;
    return false;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorPoolByType::GetDescriptor(BindingInfo info, Resource::Ptr res)
{
    size_t offset = GetOffset(info, res);
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_heap->GetCPUDescriptorHandleForHeapStart(),
        offset,
        m_context.device->GetDescriptorHandleIncrementSize(m_type));
}

size_t DescriptorPoolByType::CreateOffset()
{
    if (m_tail + 1 > m_size)
    {
        ComPtr<ID3D12DescriptorHeap> new_heap;
        D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
        heap_desc.NumDescriptors = 2 * (m_size + 1);
        heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heap_desc.Type = m_type;
        ASSERT_SUCCEEDED(m_context.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&new_heap)));
        if (m_size > 0)
        {
            m_context.device->CopyDescriptorsSimple(
                m_size,
                new_heap->GetCPUDescriptorHandleForHeapStart(),
                m_heap->GetCPUDescriptorHandleForHeapStart(),
                m_type);
        }
        m_size = heap_desc.NumDescriptors;
        m_heap = new_heap;
    }
    return m_tail++;
}

size_t DescriptorPoolByType::GetOffset(BindingInfo info, Resource::Ptr res)
{
    auto it = m_descriptor_offset.find({ info, res });
    if (it != m_descriptor_offset.end())
        return it->second;
    else
        return m_descriptor_offset[{ info, res }] = CreateOffset();
}

DescriptorPool::DescriptorPool(DX12Context& context)
    : m_context(context)
    , m_resource(m_context, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    , m_sampler(m_context, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
{
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorPool::GetDescriptor(ResourceType res_type, BindingInfo info, Resource::Ptr res)
{
    DescriptorPoolByType& pool = SelectHeap(res_type);
    return pool.GetDescriptor(info, res);
}

bool DescriptorPool::HasDescriptor(ResourceType res_type, BindingInfo info, Resource::Ptr res)
{
    DescriptorPoolByType& pool = SelectHeap(res_type);
    return pool.HasDescriptor(info, res);
}

DescriptorPoolByType& DescriptorPool::SelectHeap(ResourceType res_type)
{
    switch (res_type)
    {
    case ResourceType::kSrv:
    case ResourceType::kUav:
        return m_resource;
        break;
    case ResourceType::kSampler:
        return m_sampler;
        break;
    default:
    {
        throw "fatal failure";
    }
    }
}

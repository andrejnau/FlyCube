#include "Context/DescriptorPool.h"
#include "Context/DX12Context.h"
#include <d3dx12.h>

DescriptorHeapRange::DescriptorHeapRange(ComPtr<ID3D12DescriptorHeap>& heap, size_t offset, size_t size, size_t increment_size)
    : m_heap(heap)
    , m_offset(offset)
    , m_size(size)
    , m_increment_size(increment_size)
{
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapRange::GetCpuHandle(size_t offset) const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_heap->GetCPUDescriptorHandleForHeapStart(),
        m_offset + offset,
        m_increment_size);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapRange::GetGpuHandle(size_t offset) const
{
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(
        m_heap->GetGPUDescriptorHandleForHeapStart(),
        m_offset + offset,
        m_increment_size);
}

DescriptorHeapAllocator::DescriptorHeapAllocator(DX12Context& context, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
    : m_context(context)
    , m_type(type)
    , m_flags(flags)
    , m_offset(0)
    , m_size(0)
{
}

DescriptorHeapRange DescriptorHeapAllocator::Allocate(size_t count)
{
    if (m_offset + count > m_size)
        ResizeHeap(std::max(m_offset + count, 2 * (m_size + 1)));
    m_offset += count;
    return DescriptorHeapRange(m_heap, m_offset - count, count, m_context.device->GetDescriptorHandleIncrementSize(m_type));
}

void DescriptorHeapAllocator::ResizeHeap(size_t req_size)
{
    ComPtr<ID3D12DescriptorHeap> heap;
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = req_size;
    heap_desc.Flags = m_flags;
    heap_desc.Type = m_type;
    ASSERT_SUCCEEDED(m_context.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap)));
    if (m_size > 0)
    {
        m_context.device->CopyDescriptorsSimple(
            m_size,
            heap->GetCPUDescriptorHandleForHeapStart(),
            m_heap->GetCPUDescriptorHandleForHeapStart(),
            m_type);
    }
    m_size = heap_desc.NumDescriptors;
    m_heap = heap;
}

DescriptorPoolByType::DescriptorPoolByType(DX12Context& context, D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_context(context)
    , m_heap_alloc(context, type, D3D12_DESCRIPTOR_HEAP_FLAG_NONE)
{
}

bool DescriptorPoolByType::HasDescriptor(BindingInfo info, Resource::Ptr res)
{
    auto it = m_descriptors.find({ info, res });
    if (it != m_descriptors.end())
        return true;
    return false;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorPoolByType::GetDescriptor(BindingInfo info, Resource::Ptr res)
{
    auto it = m_descriptors.find({ info, res });
    if (it == m_descriptors.end())
    {
        it = m_descriptors.emplace(std::piecewise_construct,
            std::forward_as_tuple(info, res),
            std::forward_as_tuple(m_heap_alloc.Allocate(1))).first;
    }
    return it->second.GetCpuHandle();
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

#include "Context/DX12View.h"

#include <assert.h>

#include "Context/DX12Context.h"

DX12View::DX12View(
    D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle,
    size_t offset,
    uint32_t increment_size,
    D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_cpu_handle(cpu_handle)
    , m_offset(offset)
    , m_increment_size(increment_size)
    , m_type(type)
{
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12View::GetCpuHandle() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_cpu_handle,
        static_cast<INT>(m_offset),
        m_increment_size);
}

DX12ViewAllocator::DX12ViewAllocator(DX12Context& context, D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_context(context)
    , m_type(type)
    , m_offset(0)
    , m_size(0)
{
}

DX12View::Ptr DX12ViewAllocator::Allocate()
{
    if (m_offset + 1 > m_size)
    {
        ResizeHeap(std::max(m_offset + 1, 2 * (m_size + 1)));
    }
    ++m_offset;
    return std::make_shared<DX12View>(m_cpu_handle, m_offset - 1, m_context.device->GetDescriptorHandleIncrementSize(m_type), m_type);
}

void DX12ViewAllocator::ResizeHeap(size_t req_size)
{
    if (m_size >= req_size)
        return;

    ComPtr<ID3D12DescriptorHeap> heap;
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = static_cast<UINT>(req_size);
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heap_desc.Type = m_type;
    ASSERT_SUCCEEDED(m_context.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap)));
    if (m_size > 0)
    {
        m_context.device->CopyDescriptorsSimple(
            static_cast<UINT>(m_size),
            heap->GetCPUDescriptorHandleForHeapStart(),
            m_heap->GetCPUDescriptorHandleForHeapStart(),
            m_type);
    }

    m_size = heap_desc.NumDescriptors;
    m_heap = heap;
    m_cpu_handle = m_heap->GetCPUDescriptorHandleForHeapStart();
}

DX12ViewPool::DX12ViewPool(DX12Context& context)
    : m_context(context)
    , m_resource(m_context, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    , m_sampler(m_context, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    , m_rtv(m_context, D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
    , m_dsv(m_context, D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
{
}

DX12View::Ptr DX12ViewPool::AllocateDescriptor(ResourceType res_type)
{
    DX12ViewAllocator& view_allocator = SelectAllocator(res_type);
    return view_allocator.Allocate();
}

DX12ViewAllocator& DX12ViewPool::SelectAllocator(ResourceType res_type)
{
    switch (res_type)
    {
    case ResourceType::kSrv:
    case ResourceType::kUav:
    case ResourceType::kCbv:
        return m_resource;
    case ResourceType::kSampler:
        return m_sampler;
    case ResourceType::kRtv:
        return m_rtv;
    case ResourceType::kDsv:
        return m_dsv;
    default:
    {
        throw "fatal failure";
    }
    }
}

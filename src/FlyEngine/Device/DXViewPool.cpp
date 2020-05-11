#include "Device/DXViewPool.h"
#include "Device/DXDevice.h"
#include <d3dx12.h>

DXDescriptorHandle::DXDescriptorHandle(
    DXDevice& device,
    ComPtr<ID3D12DescriptorHeap>& heap,
    D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle,
    size_t offset,
    size_t size,
    uint32_t increment_size,
    D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_device(device)
    , m_heap(heap)
    , m_cpu_handle(cpu_handle)
    , m_offset(offset)
    , m_size(size)
    , m_increment_size(increment_size)
    , m_type(type)
{
}

D3D12_CPU_DESCRIPTOR_HANDLE DXDescriptorHandle::GetCpuHandle(size_t offset) const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_cpu_handle,
        static_cast<INT>(m_offset + offset),
        m_increment_size);
}

DXViewAllocator::DXViewAllocator(DXDevice& device, D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_device(device)
    , m_type(type)
    , m_offset(0)
    , m_size(0)
{
}

std::shared_ptr<DXDescriptorHandle> DXViewAllocator::Allocate(size_t count)
{
    if (m_offset + count > m_size)
        ResizeHeap(std::max(m_offset + count, 2 * (m_size + 1)));
    m_offset += count;
    return std::make_shared<DXDescriptorHandle>(m_device, m_heap, m_cpu_handle, m_offset - count, count, m_device.GetDevice()->GetDescriptorHandleIncrementSize(m_type), m_type);
}

void DXViewAllocator::ResizeHeap(size_t req_size)
{
    if (m_size >= req_size)
        return;

    ComPtr<ID3D12DescriptorHeap> heap;
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = static_cast<uint32_t>(req_size);
    heap_desc.Type = m_type;
    ASSERT_SUCCEEDED(m_device.GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap)));
    if (m_size > 0)
    {
        m_device.GetDevice()->CopyDescriptorsSimple(
            static_cast<uint32_t>(m_size),
            heap->GetCPUDescriptorHandleForHeapStart(),
            m_heap->GetCPUDescriptorHandleForHeapStart(),
            m_type);
    }

    m_size = heap_desc.NumDescriptors;
    m_heap = heap;
    m_cpu_handle = m_heap->GetCPUDescriptorHandleForHeapStart();
    m_gpu_handle = m_heap->GetGPUDescriptorHandleForHeapStart();
}

DXViewPool::DXViewPool(DXDevice& device)
    : m_device(device)
    , m_resource(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    , m_sampler(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    , m_rtv(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
    , m_dsv(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
{
}

std::shared_ptr<DXDescriptorHandle> DXViewPool::AllocateDescriptor(ResourceType res_type)
{
    DXViewAllocator& pool = SelectHeap(res_type);
    return pool.Allocate(1);
}

DXViewAllocator& DXViewPool::SelectHeap(ResourceType res_type)
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

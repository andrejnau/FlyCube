#include "CPUDescriptorPool/DXCPUDescriptorPoolTyped.h"

#include "Device/DXDevice.h"

#include <directx/d3dx12.h>

DXCPUDescriptorPoolTyped::DXCPUDescriptorPoolTyped(DXDevice& device, D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_device(device)
    , m_type(type)
    , m_offset(0)
    , m_size(0)
{
}

std::shared_ptr<DXCPUDescriptorHandle> DXCPUDescriptorPoolTyped::Allocate(size_t count)
{
    if (m_offset + count > m_size) {
        ResizeHeap(std::max(m_offset + count, 2 * (m_size + 1)));
    }
    m_offset += count;
    return std::make_shared<DXCPUDescriptorHandle>(m_device, m_heap, m_cpu_handle, m_offset - count, count,
                                                   m_device.GetDevice()->GetDescriptorHandleIncrementSize(m_type),
                                                   m_type);
}

void DXCPUDescriptorPoolTyped::ResizeHeap(size_t req_size)
{
    if (m_size >= req_size) {
        return;
    }

    ComPtr<ID3D12DescriptorHeap> heap;
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = static_cast<uint32_t>(req_size);
    heap_desc.Type = m_type;
    ASSERT_SUCCEEDED(m_device.GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap)));
    if (m_size > 0) {
        m_device.GetDevice()->CopyDescriptorsSimple(static_cast<uint32_t>(m_size),
                                                    heap->GetCPUDescriptorHandleForHeapStart(),
                                                    m_heap->GetCPUDescriptorHandleForHeapStart(), m_type);
    }

    m_size = heap_desc.NumDescriptors;
    m_heap = heap;
    m_cpu_handle = m_heap->GetCPUDescriptorHandleForHeapStart();
}

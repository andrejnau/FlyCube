#include "GPUDescriptorPool/DXGPUDescriptorPoolTyped.h"

#include "Device/DXDevice.h"

#include <directx/d3dx12.h>

DXGPUDescriptorPoolTyped::DXGPUDescriptorPoolTyped(DXDevice& device, D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_device(device)
    , m_type(type)
    , m_offset(0)
    , m_size(0)
{
}

DXGPUDescriptorPoolRange DXGPUDescriptorPoolTyped::Allocate(size_t count)
{
    auto it = m_empty_ranges.lower_bound(count);
    if (it != m_empty_ranges.end()) {
        size_t offset = it->second;
        size_t size = it->first;
        m_empty_ranges.erase(it);
        return DXGPUDescriptorPoolRange(*this, m_device, m_heap, m_cpu_handle, m_gpu_handle, m_heap_readable,
                                        m_cpu_handle_readable, offset, size,
                                        m_device.GetDevice()->GetDescriptorHandleIncrementSize(m_type), m_type);
    }
    if (m_offset + count > m_size) {
        ResizeHeap(std::max(m_offset + count, 2 * (m_size + 1)));
    }
    m_offset += count;
    return DXGPUDescriptorPoolRange(*this, m_device, m_heap, m_cpu_handle, m_gpu_handle, m_heap_readable,
                                    m_cpu_handle_readable, m_offset - count, count,
                                    m_device.GetDevice()->GetDescriptorHandleIncrementSize(m_type), m_type);
}

void DXGPUDescriptorPoolTyped::ResizeHeap(size_t req_size)
{
    if (m_size >= req_size) {
        return;
    }

    ComPtr<ID3D12DescriptorHeap> heap;
    ComPtr<ID3D12DescriptorHeap> heap_readable;
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = static_cast<uint32_t>(req_size);
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heap_desc.Type = m_type;
    ASSERT_SUCCEEDED(m_device.GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap)));

    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ASSERT_SUCCEEDED(m_device.GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap_readable)));

    if (m_size > 0) {
        m_device.GetDevice()->CopyDescriptorsSimple(m_size, heap_readable->GetCPUDescriptorHandleForHeapStart(),
                                                    m_cpu_handle_readable, m_type);
        m_device.GetDevice()->CopyDescriptorsSimple(m_size, heap->GetCPUDescriptorHandleForHeapStart(),
                                                    m_cpu_handle_readable, m_type);
    }

    m_size = heap_desc.NumDescriptors;
    m_heap = heap;
    m_heap_readable = heap_readable;
    m_cpu_handle = m_heap->GetCPUDescriptorHandleForHeapStart();
    m_gpu_handle = m_heap->GetGPUDescriptorHandleForHeapStart();
    m_cpu_handle_readable = m_heap_readable->GetCPUDescriptorHandleForHeapStart();
}

void DXGPUDescriptorPoolTyped::OnRangeDestroy(size_t offset, size_t size)
{
    m_empty_ranges.emplace(size, offset);
}

void DXGPUDescriptorPoolTyped::ResetHeap()
{
    m_offset = 0;
}

ComPtr<ID3D12DescriptorHeap> DXGPUDescriptorPoolTyped::GetHeap()
{
    return m_heap;
}

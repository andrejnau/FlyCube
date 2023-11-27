#include "GPUDescriptorPool/DXGPUDescriptorPoolRange.h"

#include "Device/DXDevice.h"
#include "GPUDescriptorPool/DXGPUDescriptorPoolTyped.h"

#include <directx/d3dx12.h>

DXGPUDescriptorPoolRange::DXGPUDescriptorPoolRange(DXGPUDescriptorPoolTyped& pool,
                                                   DXDevice& device,
                                                   ComPtr<ID3D12DescriptorHeap>& heap,
                                                   D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle,
                                                   D3D12_GPU_DESCRIPTOR_HANDLE& gpu_handle,
                                                   ComPtr<ID3D12DescriptorHeap>& heap_readable,
                                                   D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle_readable,
                                                   size_t offset,
                                                   size_t size,
                                                   uint32_t increment_size,
                                                   D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_pool(pool)
    , m_device(device)
    , m_heap(heap)
    , m_cpu_handle(cpu_handle)
    , m_gpu_handle(gpu_handle)
    , m_heap_readable(heap_readable)
    , m_cpu_handle_readable(cpu_handle_readable)
    , m_offset(offset)
    , m_size(size)
    , m_increment_size(increment_size)
    , m_type(type)
    , m_callback(this, [m_offset = m_offset, m_size = m_size, m_pool = m_pool](auto) {
        m_pool.get().OnRangeDestroy(m_offset, m_size);
    })
{
}

DXGPUDescriptorPoolRange::DXGPUDescriptorPoolRange(DXGPUDescriptorPoolRange&& oth) = default;

DXGPUDescriptorPoolRange::~DXGPUDescriptorPoolRange() = default;

void DXGPUDescriptorPoolRange::CopyCpuHandle(size_t dst_offset, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    D3D12_CPU_DESCRIPTOR_HANDLE self = GetCpuHandle(m_cpu_handle, dst_offset);
    m_device.get().GetDevice()->CopyDescriptors(1, &self, nullptr, 1, &handle, nullptr, m_type);
    self = GetCpuHandle(m_cpu_handle_readable, dst_offset);
    m_device.get().GetDevice()->CopyDescriptors(1, &self, nullptr, 1, &handle, nullptr, m_type);
}

D3D12_CPU_DESCRIPTOR_HANDLE DXGPUDescriptorPoolRange::GetCpuHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle,
                                                                   size_t offset) const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(handle, static_cast<INT>(m_offset + offset), m_increment_size);
}

D3D12_GPU_DESCRIPTOR_HANDLE DXGPUDescriptorPoolRange::GetGpuHandle(size_t offset) const
{
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_gpu_handle, static_cast<INT>(m_offset + offset), m_increment_size);
}

const ComPtr<ID3D12DescriptorHeap>& DXGPUDescriptorPoolRange::GetHeap() const
{
    return m_heap;
}

size_t DXGPUDescriptorPoolRange::GetSize() const
{
    return m_size;
}

size_t DXGPUDescriptorPoolRange::GetOffset() const
{
    return m_offset;
}

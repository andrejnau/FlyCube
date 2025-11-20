#include "CPUDescriptorPool/DXCPUDescriptorHandle.h"

#include "Device/DXDevice.h"

#include <directx/d3dx12.h>

DXCPUDescriptorHandle::DXCPUDescriptorHandle(DXDevice& device,
                                             ComPtr<ID3D12DescriptorHeap>& heap,
                                             D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle,
                                             size_t offset,
                                             size_t size,
                                             uint32_t increment_size,
                                             D3D12_DESCRIPTOR_HEAP_TYPE type)
    : device_(device)
    , heap_(heap)
    , cpu_handle_(cpu_handle)
    , offset_(offset)
    , size_(size)
    , increment_size_(increment_size)
    , type_(type)
{
}

D3D12_CPU_DESCRIPTOR_HANDLE DXCPUDescriptorHandle::GetCpuHandle(size_t offset) const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpu_handle_, static_cast<INT>(offset_ + offset), increment_size_);
}

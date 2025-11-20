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
    : pool_(pool)
    , device_(device)
    , heap_(heap)
    , cpu_handle_(cpu_handle)
    , gpu_handle_(gpu_handle)
    , heap_readable_(heap_readable)
    , cpu_handle_readable_(cpu_handle_readable)
    , offset_(offset)
    , size_(size)
    , increment_size_(increment_size)
    , type_(type)
    , callback_(this,
                [offset_ = offset_, size_ = size_, pool_ = pool_](auto) { pool_.get().OnRangeDestroy(offset_, size_); })
{
}

DXGPUDescriptorPoolRange::DXGPUDescriptorPoolRange(DXGPUDescriptorPoolRange&& oth) = default;

DXGPUDescriptorPoolRange::~DXGPUDescriptorPoolRange() = default;

void DXGPUDescriptorPoolRange::CopyCpuHandle(size_t dst_offset, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    D3D12_CPU_DESCRIPTOR_HANDLE self = GetCpuHandle(cpu_handle_, dst_offset);
    device_.get().GetDevice()->CopyDescriptors(1, &self, nullptr, 1, &handle, nullptr, type_);
    self = GetCpuHandle(cpu_handle_readable_, dst_offset);
    device_.get().GetDevice()->CopyDescriptors(1, &self, nullptr, 1, &handle, nullptr, type_);
}

D3D12_CPU_DESCRIPTOR_HANDLE DXGPUDescriptorPoolRange::GetCpuHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle,
                                                                   size_t offset) const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(handle, static_cast<INT>(offset_ + offset), increment_size_);
}

D3D12_GPU_DESCRIPTOR_HANDLE DXGPUDescriptorPoolRange::GetGpuHandle(size_t offset) const
{
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(gpu_handle_, static_cast<INT>(offset_ + offset), increment_size_);
}

const ComPtr<ID3D12DescriptorHeap>& DXGPUDescriptorPoolRange::GetHeap() const
{
    return heap_;
}

size_t DXGPUDescriptorPoolRange::GetSize() const
{
    return size_;
}

size_t DXGPUDescriptorPoolRange::GetOffset() const
{
    return offset_;
}

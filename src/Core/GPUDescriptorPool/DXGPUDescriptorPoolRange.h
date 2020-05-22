#pragma once
#include <Instance/BaseTypes.h>
#include <Utilities/DXUtility.h>
#include <algorithm>
#include <map>
#include <memory>
#include <wrl.h>
#include <d3d12.h>
using namespace Microsoft::WRL;

class DXDevice;

class DXGPUDescriptorPoolRange
{
public:
    using Ptr = std::shared_ptr<DXGPUDescriptorPoolRange>;
    DXGPUDescriptorPoolRange(
        DXDevice& device,
        ComPtr<ID3D12DescriptorHeap>& heap,
        D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle,
        D3D12_GPU_DESCRIPTOR_HANDLE& gpu_handle,
        ComPtr<ID3D12DescriptorHeap>& heap_readable,
        D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle_readable,
        size_t offset,
        size_t size,
        uint32_t increment_size,
        D3D12_DESCRIPTOR_HEAP_TYPE type);
    void CopyCpuHandle(size_t dst_offset, D3D12_CPU_DESCRIPTOR_HANDLE handle);
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(size_t offset = 0) const;

    const ComPtr<ID3D12DescriptorHeap>& GetHeap() const;

    size_t GetSize() const;

private:
    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle, size_t offset = 0) const;

    std::reference_wrapper<DXDevice> m_device;
    std::reference_wrapper<ComPtr<ID3D12DescriptorHeap>> m_heap;
    std::reference_wrapper<D3D12_CPU_DESCRIPTOR_HANDLE> m_cpu_handle;
    std::reference_wrapper<D3D12_GPU_DESCRIPTOR_HANDLE> m_gpu_handle;
    std::reference_wrapper<ComPtr<ID3D12DescriptorHeap>> m_heap_readable;
    std::reference_wrapper<D3D12_CPU_DESCRIPTOR_HANDLE> m_cpu_handle_readable;
    size_t m_offset;
    size_t m_size;
    uint32_t m_increment_size;
    D3D12_DESCRIPTOR_HEAP_TYPE m_type;
};

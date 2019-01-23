#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <algorithm>
#include <map>
#include <memory>

#include <Utilities/DXUtility.h>
#include "Context/BaseTypes.h"
#include "Resource/DX12Resource.h"
#include "Context/View.h"

using namespace Microsoft::WRL;

class DX12Context;

class DescriptorHeapRange : public View
{
public:
    using Ptr = std::shared_ptr<DescriptorHeapRange>;
    DescriptorHeapRange(
        DX12Context& context,
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

    std::reference_wrapper<DX12Context> m_context;
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

class DescriptorHeapAllocator
{
public:
    DescriptorHeapAllocator(DX12Context& context, D3D12_DESCRIPTOR_HEAP_TYPE type);
    DescriptorHeapRange Allocate(size_t count);
    void ResizeHeap(size_t req_size);
    void ResetHeap();

private:
    DX12Context& m_context;
    D3D12_DESCRIPTOR_HEAP_TYPE m_type;
    size_t m_offset;
    size_t m_size;
    ComPtr<ID3D12DescriptorHeap> m_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_handle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_handle;
    ComPtr<ID3D12DescriptorHeap> m_heap_readable;
    D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_handle_readable;
};

class DescriptorPool
{
public:
    DescriptorPool(DX12Context& context);
    DescriptorHeapRange Allocate(D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type, size_t count);

private:
    DX12Context& m_context;
    DescriptorHeapAllocator m_shader_resource;
    DescriptorHeapAllocator m_shader_sampler;
};

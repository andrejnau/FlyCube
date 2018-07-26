#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <algorithm>
#include <map>

#include <Utilities/DXUtility.h>
#include "Context/BaseTypes.h"
#include "Context/DX12Resource.h"

using namespace Microsoft::WRL;

class DX12Context;

class DescriptorHeapRange
{
public:
    DescriptorHeapRange(
        DX12Context& context,
        ComPtr<ID3D12DescriptorHeap>& heap,
        D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle,
        D3D12_GPU_DESCRIPTOR_HANDLE& gpu_handle,
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& copied_handle,
        size_t offset,
        size_t size,
        uint32_t increment_size,
        D3D12_DESCRIPTOR_HEAP_TYPE type);
    void CopyCpuHandle(size_t dst_offset, D3D12_CPU_DESCRIPTOR_HANDLE handle);
    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(size_t offset = 0) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(size_t offset = 0) const;

    const ComPtr<ID3D12DescriptorHeap>& GetHeap() const
    {
        return m_heap;
    }

    size_t GetSize() const
    {
        return m_size;
    }

    void SetInit()
    {
        *m_is_init = true;
    }

    bool IsInit() const
    {
        return *m_is_init;
    }

private:
    std::reference_wrapper<DX12Context> m_context;
    std::reference_wrapper<ComPtr<ID3D12DescriptorHeap>> m_heap;
    std::reference_wrapper<D3D12_CPU_DESCRIPTOR_HANDLE> m_cpu_handle;
    std::reference_wrapper<D3D12_GPU_DESCRIPTOR_HANDLE> m_gpu_handle;
    std::reference_wrapper<std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>> m_copied_handle;
    std::shared_ptr<bool> m_is_init = std::make_shared<bool>(false);
    size_t m_offset;
    size_t m_size;
    uint32_t m_increment_size;
    D3D12_DESCRIPTOR_HEAP_TYPE m_type;
};

class DescriptorHeapAllocator
{
public:
    DescriptorHeapAllocator(DX12Context& context, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags);
    DescriptorHeapRange Allocate(size_t count);
    void ResizeHeap(size_t req_size);
    void ResetHeap();

private:
    DX12Context& m_context;
    D3D12_DESCRIPTOR_HEAP_TYPE m_type;
    D3D12_DESCRIPTOR_HEAP_FLAGS m_flags;
    size_t m_offset;
    size_t m_size;
    ComPtr<ID3D12DescriptorHeap> m_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_handle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_handle;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_copied_handle;
};

class DescriptorPool
{
public:
    DescriptorPool(DX12Context& context);
    DescriptorHeapRange GetDescriptor(const BindKey& bind_key, DX12Resource& res);
    DescriptorHeapRange GetEmptyDescriptor(ResourceType res_type);
    DescriptorHeapRange AllocateDescriptor(ResourceType res_type);
    void OnFrameBegin();
    void ReqFrameDescription(ResourceType res_type, size_t count);
    DescriptorHeapRange Allocate(ResourceType res_type, size_t count);

private:
    DescriptorHeapAllocator& SelectHeap(ResourceType res_type);

    DX12Context& m_context;
    DescriptorHeapAllocator m_resource;
    DescriptorHeapAllocator m_sampler;
    DescriptorHeapAllocator m_rtv;
    DescriptorHeapAllocator m_dsv;
    DescriptorHeapAllocator m_shader_resource;
    DescriptorHeapAllocator m_shader_sampler;
    size_t m_need_resources = 0;
    size_t m_need_samplers = 0;
    bool first_frame_alloc = true;
};

#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <algorithm>
#include <map>
#include <memory>

#include <Utilities/DXUtility.h>
#include "Context/BaseTypes.h"
#include "Context/DX12Resource.h"
#include "Context/View.h"

using namespace Microsoft::WRL;

class DX12Context;

class DX12View : public View
{
public:
    using Ptr = std::shared_ptr<DX12View>;
    DX12View(
        DX12Context& context,
        ComPtr<ID3D12DescriptorHeap>& heap,
        D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle,
        size_t offset,
        size_t size,
        uint32_t increment_size,
        D3D12_DESCRIPTOR_HEAP_TYPE type);
    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(size_t offset = 0) const;

private:
    std::reference_wrapper<DX12Context> m_context;
    std::reference_wrapper<ComPtr<ID3D12DescriptorHeap>> m_heap;
    std::reference_wrapper<D3D12_CPU_DESCRIPTOR_HANDLE> m_cpu_handle;
    size_t m_offset;
    size_t m_size;
    uint32_t m_increment_size;
    D3D12_DESCRIPTOR_HEAP_TYPE m_type;
};

class DX12ViewAllocator
{
public:
    DX12ViewAllocator(DX12Context& context, D3D12_DESCRIPTOR_HEAP_TYPE type);
    DX12View::Ptr Allocate(size_t count);
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
};

class DX12ViewPool
{
public:
    DX12ViewPool(DX12Context& context);
    DX12View::Ptr AllocateDescriptor(ResourceType res_type);

private:
    DX12ViewAllocator& SelectHeap(ResourceType res_type);

    DX12Context& m_context;
    DX12ViewAllocator m_resource;
    DX12ViewAllocator m_sampler;
    DX12ViewAllocator m_rtv;
    DX12ViewAllocator m_dsv;
};

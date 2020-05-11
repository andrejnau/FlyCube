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

class DXDescriptorHandle
{
public:
    DXDescriptorHandle(
        DXDevice& device,
        ComPtr<ID3D12DescriptorHeap>& heap,
        D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle,
        size_t offset,
        size_t size,
        uint32_t increment_size,
        D3D12_DESCRIPTOR_HEAP_TYPE type);
    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(size_t offset = 0) const;

private:
    std::reference_wrapper<DXDevice> m_device;
    std::reference_wrapper<ComPtr<ID3D12DescriptorHeap>> m_heap;
    std::reference_wrapper<D3D12_CPU_DESCRIPTOR_HANDLE> m_cpu_handle;
    size_t m_offset;
    size_t m_size;
    uint32_t m_increment_size;
    D3D12_DESCRIPTOR_HEAP_TYPE m_type;
};

class DXViewAllocator
{
public:
    DXViewAllocator(DXDevice& device, D3D12_DESCRIPTOR_HEAP_TYPE type);
    std::shared_ptr<DXDescriptorHandle> Allocate(size_t count);
    void ResizeHeap(size_t req_size);

private:
    DXDevice& m_device;
    D3D12_DESCRIPTOR_HEAP_TYPE m_type;
    size_t m_offset;
    size_t m_size;
    ComPtr<ID3D12DescriptorHeap> m_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_handle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_handle;
};

class DXViewPool
{
public:
    DXViewPool(DXDevice& device);
    std::shared_ptr<DXDescriptorHandle> AllocateDescriptor(ResourceType res_type);

private:
    DXViewAllocator& SelectHeap(ResourceType res_type);

    DXDevice& m_device;
    DXViewAllocator m_resource;
    DXViewAllocator m_sampler;
    DXViewAllocator m_rtv;
    DXViewAllocator m_dsv;
};

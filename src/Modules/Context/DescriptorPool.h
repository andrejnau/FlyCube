#pragma once

#include <d3dcompiler.h>
#include <d3d12.h>

#include <wrl.h>
using namespace Microsoft::WRL;

#include <map>
#include "Context/BaseTypes.h"
#include "Context/Resource.h"

#include <Utilities/DXUtility.h>
#include <algorithm>

class DX12Context;

struct BindingInfo
{
    D3D_SRV_DIMENSION dimension;
    bool operator<(const BindingInfo& oth) const
    {
        return dimension < oth.dimension;
    }
};

class DescriptorHeapRange
{
public:
    DescriptorHeapRange(ComPtr<ID3D12DescriptorHeap>& heap, size_t offset, size_t size, size_t increment_size);
    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(size_t offset = 0) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(size_t offset = 0) const;

private:
    ComPtr<ID3D12DescriptorHeap>& m_heap;
    size_t m_offset;
    size_t m_size;
    size_t m_increment_size;
};

class DescriptorHeapAllocator
{
public:
    DescriptorHeapAllocator(DX12Context& context, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags);
    DescriptorHeapRange Allocate(size_t count);

private:
    void ResizeHeap(size_t req_size);

    DX12Context& m_context;
    D3D12_DESCRIPTOR_HEAP_TYPE m_type;
    D3D12_DESCRIPTOR_HEAP_FLAGS m_flags;
    size_t m_offset;
    size_t m_size;
    ComPtr<ID3D12DescriptorHeap> m_heap;
};

struct DescriptorPoolByType
{
public:
    DescriptorPoolByType(DX12Context& context, D3D12_DESCRIPTOR_HEAP_TYPE type);
    bool HasDescriptor(BindingInfo info, Resource::Ptr res);
    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptor(BindingInfo info, Resource::Ptr res);

private:
    DX12Context& m_context;
    DescriptorHeapAllocator m_heap_alloc;
    std::map<std::tuple<BindingInfo, Resource::Ptr>, DescriptorHeapRange> m_descriptors;
};

class DescriptorPool
{
public:
    DescriptorPool(DX12Context& context);
    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptor(ResourceType res_type, BindingInfo info, Resource::Ptr res);
    bool HasDescriptor(ResourceType res_type, BindingInfo info, Resource::Ptr res);

private:
    DescriptorPoolByType& SelectHeap(ResourceType res_type);

    DX12Context& m_context;
    DescriptorPoolByType m_resource;
    DescriptorPoolByType m_sampler;
};

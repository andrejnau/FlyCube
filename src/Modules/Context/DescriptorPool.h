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

class DescriptorHeapRange
{
public:
    DescriptorHeapRange(ComPtr<ID3D12DescriptorHeap>& heap, size_t offset, size_t size, size_t increment_size, D3D12_DESCRIPTOR_HEAP_TYPE type);
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

private:
    ComPtr<ID3D12DescriptorHeap>& m_heap;
    size_t m_offset;
    size_t m_size;
    size_t m_increment_size;
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
};

struct DescriptorPoolByType
{
public:
    DescriptorPoolByType(DX12Context& context, D3D12_DESCRIPTOR_HEAP_TYPE type);
    bool HasDescriptor(size_t bind_id, const ComPtr<ID3D12Resource>& res);
    DescriptorHeapRange GetDescriptor(size_t bind_id, const ComPtr<ID3D12Resource>& res);

private:
    DX12Context& m_context;
    DescriptorHeapAllocator m_heap_alloc;
    std::map<std::tuple<size_t, ComPtr<ID3D12Resource>>, DescriptorHeapRange> m_descriptors;
};

class DescriptorPool
{
public:
    DescriptorPool(DX12Context& context);
    DescriptorHeapRange GetDescriptor(ResourceType res_type, size_t bind_id, const ComPtr<ID3D12Resource>& res);
    bool HasDescriptor(ResourceType res_type, size_t bind_id, const ComPtr<ID3D12Resource>& res);
    void OnFrameBegin();
    void ReqFrameDescription(ResourceType res_type, size_t count);
    DescriptorHeapRange Allocate(ResourceType res_type, size_t count);

private:
    DescriptorPoolByType& SelectHeap(ResourceType res_type);

    DX12Context& m_context;
    DescriptorPoolByType m_resource;
    DescriptorPoolByType m_sampler;
    DescriptorPoolByType m_rtv;
    DescriptorPoolByType m_dsv;
    DescriptorHeapAllocator m_shader_resource;
    DescriptorHeapAllocator m_shader_sampler;
    size_t m_need_resources = 0;
    size_t m_need_samplers = 0;
    bool first_frame_alloc = true;
};

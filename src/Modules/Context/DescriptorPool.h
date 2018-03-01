#pragma once

#include <d3dcompiler.h>
#include <d3d12.h>

#include <wrl.h>
using namespace Microsoft::WRL;

#include <map>
#include "Context/BaseTypes.h"

class DX12Context;

struct BindingInfo
{
    D3D_SRV_DIMENSION dimension;
    bool operator<(const BindingInfo& oth) const
    {
        return dimension < oth.dimension;
    }
};

struct DescriptorPoolByType
{
public:
    DescriptorPoolByType(DX12Context& context, D3D12_DESCRIPTOR_HEAP_TYPE type);
    bool HasDescriptor(BindingInfo info, ComPtr<IUnknown> res);
    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptor(BindingInfo info, ComPtr<IUnknown> res);

private:
    size_t CreateOffset();
    size_t GetOffset(BindingInfo info, ComPtr<IUnknown> res);

    DX12Context& m_context;
    D3D12_DESCRIPTOR_HEAP_TYPE m_type;
    size_t m_tail;
    size_t m_size;
    ComPtr<ID3D12DescriptorHeap> m_heap;
    std::map<std::tuple<BindingInfo, IUnknown*>, size_t> m_descriptor_offset;
};

class DescriptorPool
{
public:
    DescriptorPool(DX12Context& context);
    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptor(ResourceType res_type, BindingInfo info, ComPtr<IUnknown> res);
    bool HasDescriptor(ResourceType res_type, BindingInfo info, ComPtr<IUnknown> res);

private:
    DescriptorPoolByType& SelectHeap(ResourceType res_type);

    DX12Context& m_context;
    DescriptorPoolByType m_resource;
    DescriptorPoolByType m_sampler;
};

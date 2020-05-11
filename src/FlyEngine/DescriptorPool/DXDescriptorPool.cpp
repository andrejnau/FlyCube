#include "DescriptorPool/DXDescriptorPool.h"
#include <Device/DXDevice.h>
#include <d3dx12.h>

DXDescriptorPool::DXDescriptorPool(DXDevice& device)
    : m_device(device)
    , m_resource(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    , m_sampler(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    , m_rtv(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
    , m_dsv(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
{
}

std::shared_ptr<DXDescriptorHandle> DXDescriptorPool::AllocateDescriptor(ResourceType res_type)
{
    DXDescriptorPoolTyped& pool = SelectHeap(res_type);
    return pool.Allocate(1);
}

DXDescriptorPoolTyped& DXDescriptorPool::SelectHeap(ResourceType res_type)
{
    switch (res_type)
    {
    case ResourceType::kSrv:
    case ResourceType::kUav:
    case ResourceType::kCbv:
        return m_resource;
    case ResourceType::kSampler:
        return m_sampler;
    case ResourceType::kRtv:
        return m_rtv;
    case ResourceType::kDsv:
        return m_dsv;
    default:
        throw "fatal failure";
    }
}

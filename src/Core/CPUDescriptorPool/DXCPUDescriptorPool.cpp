#include "CPUDescriptorPool/DXCPUDescriptorPool.h"
#include <Device/DXDevice.h>
#include <d3dx12.h>

DXCPUDescriptorPool::DXCPUDescriptorPool(DXDevice& device)
    : m_device(device)
    , m_resource(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    , m_sampler(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    , m_rtv(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
    , m_dsv(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
{
}

std::shared_ptr<DXCPUDescriptorHandle> DXCPUDescriptorPool::AllocateDescriptor(ViewType view_type)
{
    DXCPUDescriptorPoolTyped& pool = SelectHeap(view_type);
    return pool.Allocate(1);
}

DXCPUDescriptorPoolTyped& DXCPUDescriptorPool::SelectHeap(ViewType view_type)
{
    switch (view_type)
    {
    case ViewType::kSrv:
    case ViewType::kUav:
    case ViewType::kCbv:
        return m_resource;
    case ViewType::kSampler:
        return m_sampler;
    case ViewType::kRtv:
        return m_rtv;
    case ViewType::kDsv:
        return m_dsv;
    default:
        throw "fatal failure";
    }
}

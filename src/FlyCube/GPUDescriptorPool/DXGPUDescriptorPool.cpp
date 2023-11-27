#include "GPUDescriptorPool/DXGPUDescriptorPool.h"

#include "Device/DXDevice.h"

#include <directx/d3dx12.h>

#include <stdexcept>

DXGPUDescriptorPool::DXGPUDescriptorPool(DXDevice& device)
    : m_device(device)
    , m_shader_resource(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    , m_shader_sampler(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
{
}

DXGPUDescriptorPoolRange DXGPUDescriptorPool::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type, size_t count)
{
    switch (descriptor_type) {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return m_shader_resource.Allocate(count);
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
        return m_shader_sampler.Allocate(count);
    default:
        throw std::runtime_error("wrong descriptor type");
    }
}

ComPtr<ID3D12DescriptorHeap> DXGPUDescriptorPool::GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type)
{
    switch (descriptor_type) {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return m_shader_resource.GetHeap();
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
        return m_shader_sampler.GetHeap();
    default:
        throw std::runtime_error("wrong descriptor type");
    }
}

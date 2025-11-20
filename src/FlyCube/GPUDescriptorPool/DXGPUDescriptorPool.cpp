#include "GPUDescriptorPool/DXGPUDescriptorPool.h"

#include "Device/DXDevice.h"
#include "Utilities/NotReached.h"

#include <directx/d3dx12.h>

DXGPUDescriptorPool::DXGPUDescriptorPool(DXDevice& device)
    : device_(device)
    , shader_resource_(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    , shader_sampler_(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
{
}

DXGPUDescriptorPoolRange DXGPUDescriptorPool::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type, size_t count)
{
    switch (descriptor_type) {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return shader_resource_.Allocate(count);
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
        return shader_sampler_.Allocate(count);
    default:
        NOTREACHED();
    }
}

ComPtr<ID3D12DescriptorHeap> DXGPUDescriptorPool::GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type)
{
    switch (descriptor_type) {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return shader_resource_.GetHeap();
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
        return shader_sampler_.GetHeap();
    default:
        NOTREACHED();
    }
}

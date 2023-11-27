#include "QueryHeap/DXRayTracingQueryHeap.h"

#include "Device/DXDevice.h"
#include "Utilities/DXUtility.h"
#include "Utilities/SystemUtils.h"

#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <dxgi1_6.h>

DXRayTracingQueryHeap::DXRayTracingQueryHeap(DXDevice& device, QueryHeapType type, uint32_t count)
    : m_device(device)
{
    auto desc = CD3DX12_RESOURCE_DESC::Buffer(count * sizeof(uint64_t));
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    m_device.GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                                  D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                  nullptr, IID_PPV_ARGS(&m_resource));
}

QueryHeapType DXRayTracingQueryHeap::GetType() const
{
    return QueryHeapType::kAccelerationStructureCompactedSize;
}

ComPtr<ID3D12Resource> DXRayTracingQueryHeap::GetResource() const
{
    return m_resource;
}

#include "QueryHeap/DXRayTracingQueryHeap.h"

#include "Device/DXDevice.h"
#include "Utilities/DXUtility.h"
#include "Utilities/SystemUtils.h"

#include <directx/d3dx12.h>

DXRayTracingQueryHeap::DXRayTracingQueryHeap(DXDevice& device, QueryHeapType type, uint32_t count)
    : device_(device)
{
    auto desc = CD3DX12_RESOURCE_DESC::Buffer(count * sizeof(uint64_t));
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    auto heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    device_.GetDevice()->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &desc,
                                                 D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resource_));
}

QueryHeapType DXRayTracingQueryHeap::GetType() const
{
    return QueryHeapType::kAccelerationStructureCompactedSize;
}

ID3D12Resource* DXRayTracingQueryHeap::GetResource() const
{
    return resource_.Get();
}

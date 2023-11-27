#pragma once
#include "QueryHeap/QueryHeap.h"

#include <directx/d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;

class DXRayTracingQueryHeap : public QueryHeap {
public:
    DXRayTracingQueryHeap(DXDevice& device, QueryHeapType type, uint32_t count);

    QueryHeapType GetType() const override;

    ComPtr<ID3D12Resource> GetResource() const;

private:
    DXDevice& m_device;
    ComPtr<ID3D12Resource> m_resource;
};

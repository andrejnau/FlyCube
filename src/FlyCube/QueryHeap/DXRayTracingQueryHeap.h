#pragma once
#include "QueryHeap/QueryHeap.h"

#if defined(_WIN32)
#include <wrl.h>
#else
#include <wsl/wrladapter.h>
#endif

#include <directx/d3d12.h>

using namespace Microsoft::WRL;

class DXDevice;

class DXRayTracingQueryHeap : public QueryHeap {
public:
    DXRayTracingQueryHeap(DXDevice& device, QueryHeapType type, uint32_t count);

    QueryHeapType GetType() const override;

    ID3D12Resource* GetResource() const;

private:
    DXDevice& m_device;
    ComPtr<ID3D12Resource> m_resource;
};

#pragma once
#include "Fence/Fence.h"

#include <directx/d3d12.h>
#include <dxgi.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;

class DXFence : public Fence {
public:
    DXFence(DXDevice& device, uint64_t initial_value);
    uint64_t GetCompletedValue() override;
    void Wait(uint64_t value) override;
    void Signal(uint64_t value) override;

    ComPtr<ID3D12Fence> GetFence();

private:
    DXDevice& m_device;
    ComPtr<ID3D12Fence> m_fence;
    HANDLE m_fence_event = nullptr;
};

#pragma once
#include "Fence/Fence.h"

#if defined(_WIN32)
#include <wrl.h>
#else
#include <wsl/wrladapter.h>
#endif

#include <directx/d3d12.h>

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

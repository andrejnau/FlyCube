#include "Fence/Fence.h"
#include <dxgi.h>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;

class DXFence : public Fence
{
public:
    DXFence(DXDevice& device);
    void WaitAndReset() override;

    ComPtr<ID3D12Fence> GetFence();
    uint64_t GetValue();

private:
    DXDevice& m_device;
    ComPtr<ID3D12Fence> m_fence;
    uint64_t m_fence_value = 0;
    HANDLE m_fence_event = nullptr;
};

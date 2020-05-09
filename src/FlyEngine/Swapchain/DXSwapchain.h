#include "Swapchain/Swapchain.h"
#include <Resource/DX12Resource.h>
#include <cstdint>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;

class DXSwapchain
    : public Swapchain
    , public DX12Resource::Destroyer
{
public:
    DXSwapchain(DXDevice& device, GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count);
    uint32_t GetCurrentBackBufferIndex() override;
    Resource::Ptr GetBackBuffer(uint32_t buffer) override;
    void Present() override;

    void QueryOnDelete(ComPtr<IUnknown> res) override;

private:
    ComPtr<IDXGISwapChain3> m_swap_chain;
    std::vector<std::shared_ptr<DX12Resource>> m_back_buffers;
};

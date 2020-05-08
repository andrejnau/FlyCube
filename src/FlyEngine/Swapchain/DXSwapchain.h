#include "Swapchain/Swapchain.h"
#include <cstdint>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;

class DXSwapchain : public Swapchain
{
public:
    DXSwapchain(DXDevice& device, GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count);

private:
    ComPtr<IDXGISwapChain3> m_swap_chain;
};

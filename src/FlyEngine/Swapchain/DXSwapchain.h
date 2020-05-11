#pragma once
#include "Swapchain/Swapchain.h"
#include <Resource/Resource.h>
#include <cstdint>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;

class DXSwapchain
    : public Swapchain
{
public:
    DXSwapchain(DXDevice& device, GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count);
    std::shared_ptr<Resource> GetBackBuffer(uint32_t buffer) override;
    uint32_t NextImage(const std::shared_ptr<Semaphore>& semaphore) override;
    void Present(const std::shared_ptr<Semaphore>& semaphore) override;

private:
    DXDevice& m_device;
    ComPtr<IDXGISwapChain3> m_swap_chain;
    std::vector<std::shared_ptr<Resource>> m_back_buffers;
};

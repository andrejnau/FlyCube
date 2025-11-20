#pragma once
#include "Resource/Resource.h"
#include "Swapchain/Swapchain.h"

#include <directx/d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <cstdint>
#include <vector>

using Microsoft::WRL::ComPtr;

class DXCommandQueue;

class DXSwapchain : public Swapchain {
public:
    DXSwapchain(DXCommandQueue& command_queue,
                const NativeSurface& surface,
                uint32_t width,
                uint32_t height,
                uint32_t frame_count,
                bool vsync);
    gli::format GetFormat() const override;
    std::shared_ptr<Resource> GetBackBuffer(uint32_t buffer) override;
    uint32_t NextImage(const std::shared_ptr<Fence>& fence, uint64_t signal_value) override;
    void Present(const std::shared_ptr<Fence>& fence, uint64_t wait_value) override;

private:
    DXCommandQueue& command_queue_;
    bool vsync_;
    ComPtr<IDXGISwapChain3> swap_chain_;
    std::vector<std::shared_ptr<Resource>> back_buffers_;
};

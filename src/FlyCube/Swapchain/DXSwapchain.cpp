#include "Swapchain/DXSwapchain.h"

#include "Adapter/DXAdapter.h"
#include "CommandQueue/DXCommandQueue.h"
#include "Device/DXDevice.h"
#include "Instance/DXInstance.h"
#include "Resource/DXResource.h"
#include "Utilities/DXUtility.h"

#include <gli/dx.hpp>

DXSwapchain::DXSwapchain(DXCommandQueue& command_queue,
                         WindowHandle window,
                         uint32_t width,
                         uint32_t height,
                         uint32_t frame_count,
                         bool vsync)
    : m_command_queue(command_queue)
    , m_vsync(vsync)
{
    DXInstance& instance = command_queue.GetDevice().GetAdapter().GetInstance();
    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.Width = width;
    swap_chain_desc.Height = height;
    swap_chain_desc.Format = static_cast<DXGI_FORMAT>(gli::dx().translate(GetFormat()).DXGIFormat.DDS);
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount = frame_count;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    ComPtr<IDXGISwapChain1> tmp_swap_chain;
    ASSERT_SUCCEEDED(instance.GetFactory()->CreateSwapChainForHwnd(command_queue.GetQueue().Get(),
                                                                   reinterpret_cast<HWND>(window), &swap_chain_desc,
                                                                   nullptr, nullptr, &tmp_swap_chain));
    ASSERT_SUCCEEDED(
        instance.GetFactory()->MakeWindowAssociation(reinterpret_cast<HWND>(window), DXGI_MWA_NO_WINDOW_CHANGES));
    tmp_swap_chain.As(&m_swap_chain);

    for (size_t i = 0; i < frame_count; ++i) {
        std::shared_ptr<DXResource> res = std::make_shared<DXResource>(command_queue.GetDevice());
        ComPtr<ID3D12Resource> back_buffer;
        ASSERT_SUCCEEDED(m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&back_buffer)));
        res->format = GetFormat();
        res->SetInitialState(ResourceState::kPresent);
        res->resource = back_buffer;
        res->desc = back_buffer->GetDesc();
        res->is_back_buffer = true;
        m_back_buffers.emplace_back(res);
    }
}

gli::format DXSwapchain::GetFormat() const
{
    return gli::FORMAT_RGBA8_UNORM_PACK8;
}

std::shared_ptr<Resource> DXSwapchain::GetBackBuffer(uint32_t buffer)
{
    return m_back_buffers[buffer];
}

uint32_t DXSwapchain::NextImage(const std::shared_ptr<Fence>& fence, uint64_t signal_value)
{
    uint32_t frame_index = m_swap_chain->GetCurrentBackBufferIndex();
    m_command_queue.Signal(fence, signal_value);
    return frame_index;
}

void DXSwapchain::Present(const std::shared_ptr<Fence>& fence, uint64_t wait_value)
{
    m_command_queue.Wait(fence, wait_value);
    if (m_vsync) {
        ASSERT_SUCCEEDED(m_swap_chain->Present(1, 0));
    } else {
        ASSERT_SUCCEEDED(m_swap_chain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
    }
}

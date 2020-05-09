#include "Device/DXDevice.h"
#include <Adapter/DXAdapter.h>
#include <Swapchain/DXSwapchain.h>
#include <CommandList/DXCommandList.h>
#include <Utilities/DXUtility.h>
#include <dxgi1_6.h>

DXDevice::DXDevice(DXAdapter& adapter)
    : m_adapter(adapter)
{
    ASSERT_SUCCEEDED(D3D12CreateDevice(m_adapter.GetAdapter().Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&m_device)));

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    ASSERT_SUCCEEDED(m_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_command_queue)));
}

std::unique_ptr<Swapchain> DXDevice::CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count)
{
    return std::make_unique<DXSwapchain>(*this, window, width, height, frame_count);
}

std::unique_ptr<CommandList> DXDevice::CreateCommandList()
{
    return std::make_unique<DXCommandList>(*this);
}

DXAdapter& DXDevice::GetAdapter()
{
    return m_adapter;
}

ComPtr<ID3D12CommandQueue> DXDevice::GetCommandQueue()
{
    return m_command_queue;
}

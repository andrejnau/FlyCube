#include "Device/DXDevice.h"
#include <Adapter/DXAdapter.h>
#include <Swapchain/DXSwapchain.h>
#include <CommandList/DXCommandList.h>
#include <Fence/DXFence.h>
#include <Utilities/DXUtility.h>
#include <dxgi1_6.h>

DXDevice::DXDevice(DXAdapter& adapter)
    : m_adapter(adapter)
{
    ASSERT_SUCCEEDED(D3D12CreateDevice(m_adapter.GetAdapter().Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&m_device)));

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    ASSERT_SUCCEEDED(m_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_command_queue)));

#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> info_queue;
    if (SUCCEEDED(m_device.As(&info_queue)))
    {
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

        D3D12_MESSAGE_SEVERITY severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO,
        };

        D3D12_MESSAGE_ID deny_ids[] =
        {
            D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES,
        };

        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumSeverities = std::size(severities);
        filter.DenyList.pSeverityList = severities;
        filter.DenyList.NumIDs = std::size(deny_ids);
        filter.DenyList.pIDList = deny_ids;
        info_queue->PushStorageFilter(&filter);
    }
#endif
}

std::unique_ptr<Swapchain> DXDevice::CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count)
{
    return std::make_unique<DXSwapchain>(*this, window, width, height, frame_count);
}

std::unique_ptr<CommandList> DXDevice::CreateCommandList()
{
    return std::make_unique<DXCommandList>(*this);
}

std::unique_ptr<Fence> DXDevice::CreateFence()
{
    return std::make_unique<DXFence>(*this);
}

void DXDevice::ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists, const std::unique_ptr<Fence>& fence)
{
    std::vector<ID3D12CommandList*> dx_command_lists;
    for (auto& command_list : command_lists)
    {
        if (!command_list)
            continue;
        DXCommandList& dx_command_list = static_cast<DXCommandList&>(*command_list);
        dx_command_lists.emplace_back(dx_command_list.GetCommandList().Get());
    }
    m_command_queue->ExecuteCommandLists(dx_command_lists.size(), dx_command_lists.data());

    if (fence)
    {
        DXFence& dx_fence = static_cast<DXFence&>(*fence);
        ASSERT_SUCCEEDED(m_command_queue->Signal(dx_fence.GetFence().Get(), dx_fence.GetValue()));
    }
}

DXAdapter& DXDevice::GetAdapter()
{
    return m_adapter;
}

ComPtr<ID3D12Device> DXDevice::GetDevice()
{
    return m_device;
}

ComPtr<ID3D12CommandQueue> DXDevice::GetCommandQueue()
{
    return m_command_queue;
}

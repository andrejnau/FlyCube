#include "Context/Context.h"
#include <Utilities/FormatHelper.h>

Context::Context(const Settings& settings, GLFWwindow* window)
    : m_window(window)
    , m_vsync(settings.vsync)
{
    m_instance = CreateInstance(settings.api_type);
    m_adapter = std::move(m_instance->EnumerateAdapters()[settings.required_gpu_index]);
    m_device = m_adapter->CreateDevice();
    m_command_queue = m_device->GetCommandQueue(CommandListType::kGraphics);

    glfwGetWindowSize(window, &m_width, &m_height);
    m_swapchain = m_device->CreateSwapchain(window, m_width, m_height, FrameCount, settings.vsync);
    m_fence = m_device->CreateFence(m_fence_value);
    for (uint32_t i = 0; i < FrameCount; ++i)
    {
        m_swapchain_command_lists.emplace_back(m_device->CreateCommandList());
        m_swapchain_fence_values.emplace_back(0);
    }
}

Context::~Context()
{
    WaitIdle();
}

std::shared_ptr<Resource> Context::GetBackBuffer(uint32_t buffer)
{
    return m_swapchain->GetBackBuffer(buffer);
}

void Context::ExecuteCommandLists(const std::vector<std::shared_ptr<CommandListBox>>& command_lists)
{
    for (auto& command_list : command_lists)
    {
        command_list->fence_value = m_fence_value + 1;
    }
    ExecuteCommandListsImpl(command_lists);
    m_command_queue->Signal(m_fence, ++m_fence_value);
}

void Context::WaitIdle()
{
    m_command_queue->Signal(m_fence, ++m_fence_value);
    m_fence->Wait(m_fence_value);
}

void Context::Resize(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    m_swapchain.reset();
    m_swapchain = m_device->CreateSwapchain(m_window, m_width, m_height, FrameCount, m_vsync);
    m_frame_index = 0;
}

void Context::Present()
{
    auto& back_buffer = GetBackBuffer(GetFrameIndex());
    auto& global_state_tracker = back_buffer->GetGlobalResourceStateTracker();
    ResourceBarrierDesc barrier = {};
    barrier.resource = back_buffer;
    barrier.state_before = global_state_tracker.GetSubresourceState(0, 0);
    barrier.state_after = ResourceState::kPresent;
    global_state_tracker.SetSubresourceState(0, 0, barrier.state_after);

    m_swapchain_command_list = m_swapchain_command_lists[m_frame_index];
    m_fence->Wait(m_swapchain_fence_values[m_frame_index]);
    m_swapchain_command_list->Reset();
    m_swapchain_command_list->ResourceBarrier({ barrier });
    m_swapchain_command_list->Close();

    m_swapchain->NextImage(m_fence, ++m_fence_value);
    m_command_queue->Wait(m_fence, m_fence_value);
    m_command_queue->ExecuteCommandLists({ m_swapchain_command_list });
    m_command_queue->Signal(m_fence, ++m_fence_value);
    m_swapchain_fence_values[m_frame_index] = m_fence_value;
    m_swapchain->Present(m_fence, m_fence_value);

    m_frame_index = (m_frame_index + 1) % FrameCount;
}

std::shared_ptr<Device> Context::GetDevice()
{
    return m_device;
}

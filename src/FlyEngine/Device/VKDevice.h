#pragma once
#include "Device/Device.h"
#include <Utilities/Vulkan.h>

class VKAdapter;

class VKDevice : public Device
{
public:
    VKDevice(VKAdapter& adapter);
    std::shared_ptr<Swapchain> CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count) override;
    std::shared_ptr<CommandList> CreateCommandList() override;
    std::shared_ptr<Fence> CreateFence() override;
    std::shared_ptr<Semaphore> CreateGPUSemaphore() override;
    std::shared_ptr<View> CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc) override;
    void Wait(const std::shared_ptr<Semaphore>& semaphore) override;
    void Signal(const std::shared_ptr<Semaphore>& semaphore) override;
    void ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists, const std::shared_ptr<Fence>& fence) override;

    VKAdapter& GetAdapter();
    uint32_t GetQueueFamilyIndex();
    vk::Device GetDevice();
    vk::Queue GetQueue();
    vk::CommandPool GetCmdPool();

private:
    VKAdapter& m_adapter;
    uint32_t m_queue_family_index = -1;
    vk::UniqueDevice m_device;
    vk::Queue m_queue;
    vk::UniqueCommandPool m_cmd_pool;
};

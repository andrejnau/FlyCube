#include "Device/Device.h"
#include <vulkan/vulkan.hpp>

class VKAdapter;

class VKDevice : public Device
{
public:
    VKDevice(VKAdapter& adapter);
    std::unique_ptr<Swapchain> CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count) override;
    std::unique_ptr<CommandList> CreateCommandList() override;
    void ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists) override;

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
    vk::UniqueFence m_fence;
};

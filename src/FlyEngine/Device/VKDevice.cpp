#include "Device/VKDevice.h"
#include <VulkanExtLoader/VulkanExtLoader.h>
#include <Swapchain/VKSwapchain.h>
#include <CommandList/VKCommandList.h>
#include <Fence/VKFence.h>
#include <Adapter/VKAdapter.h>
#include <Utilities/VKUtility.h>
#include <set>

VKDevice::VKDevice(VKAdapter& adapter)
    : m_adapter(adapter)
{
    const vk::PhysicalDevice& physical_device = adapter.GetPhysicalDevice();
    auto queue_families = physical_device.getQueueFamilyProperties();

    for (size_t i = 0; i < queue_families.size(); ++i)
    {
        const auto& queue = queue_families[i];
        if (queue.queueCount > 0 && queue.queueFlags & vk::QueueFlagBits::eGraphics && queue.queueFlags & vk::QueueFlagBits::eCompute)
        {
            m_queue_family_index = static_cast<uint32_t>(i);
            break;
        }
    }
    ASSERT(m_queue_family_index != -1);

    auto extensions = physical_device.enumerateDeviceExtensionProperties();
    std::set<std::string> req_extension = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME,
        VK_NV_RAY_TRACING_EXTENSION_NAME,
        VK_KHR_MAINTENANCE3_EXTENSION_NAME
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
    };
    std::vector<const char*> found_extension;
    for (const auto& extension : extensions)
    {
        if (req_extension.count(extension.extensionName))
            found_extension.push_back(extension.extensionName);
    }

    const float queue_priority = 1.0f;
    vk::DeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.queueFamilyIndex = m_queue_family_index;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;

    vk::PhysicalDeviceFeatures device_features = {};
    device_features.textureCompressionBC = true;
    device_features.vertexPipelineStoresAndAtomics = true;
    device_features.samplerAnisotropy = true;
    device_features.fragmentStoresAndAtomics = true;
    device_features.sampleRateShading = true;
    device_features.geometryShader = true;
    device_features.imageCubeArray = true;

    vk::DeviceCreateInfo device_create_info = {};
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = found_extension.size();
    device_create_info.ppEnabledExtensionNames = found_extension.data();

    m_device = physical_device.createDeviceUnique(device_create_info);
    LoadVkDeviceExt(m_device.get());

    m_queue = m_device->getQueue(m_queue_family_index, 0);

    vk::CommandPoolCreateInfo cmd_pool_create_info = {};
    cmd_pool_create_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    cmd_pool_create_info.queueFamilyIndex = m_queue_family_index;
    m_cmd_pool = m_device->createCommandPoolUnique(cmd_pool_create_info);
}

std::unique_ptr<Swapchain> VKDevice::CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count)
{
    return std::make_unique<VKSwapchain>(*this, window, width, height, frame_count);
}

std::unique_ptr<CommandList> VKDevice::CreateCommandList()
{
    return std::make_unique<VKCommandList>(*this);
}

std::unique_ptr<Fence> VKDevice::CreateFence()
{
    return std::make_unique<VKFence>(*this);
}

void VKDevice::ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists, const std::unique_ptr<Fence>& fence)
{
    std::vector<vk::CommandBuffer> vk_command_lists;
    for (auto& command_list : command_lists)
    {
        if (!command_list)
            continue;
        VKCommandList& vk_command_list = static_cast<VKCommandList&>(*command_list);
        vk_command_lists.emplace_back(vk_command_list.GetCommandList());
    }

    vk::SubmitInfo submit_info = {};
    submit_info.commandBufferCount = vk_command_lists.size();
    submit_info.pCommandBuffers = vk_command_lists.data();
    /*submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &m_image_available_semaphore.get();*/
    vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eTransfer;
    submit_info.pWaitDstStageMask = &waitDstStageMask;
    /*submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &m_rendering_finished_semaphore.get();*/

    vk::Fence fence_hande = {};
    if (fence)
    {
        VKFence& vk_fence = static_cast<VKFence&>(*fence);
        fence_hande = vk_fence.GetFence();
    }
    m_queue.submit(1, &submit_info, fence_hande);
}

VKAdapter& VKDevice::GetAdapter()
{
    return m_adapter;
}

uint32_t VKDevice::GetQueueFamilyIndex()
{
    return m_queue_family_index;
}

vk::Device VKDevice::GetDevice()
{
    return m_device.get();
}

vk::Queue VKDevice::GetQueue()
{
    return m_queue;
}

vk::CommandPool VKDevice::GetCmdPool()
{
    return m_cmd_pool.get();
}

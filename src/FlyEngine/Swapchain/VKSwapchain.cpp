#define GLFW_INCLUDE_VULKAN
#include "Swapchain/VKSwapchain.h"
#include <Device/VKDevice.h>
#include <Adapter/VKAdapter.h>
#include <Instance/VKInstance.h>
#include <Utilities/State.h>
#include <Utilities/VKUtility.h>
#include <Resource/VKResource.h>

VKSwapchain::VKSwapchain(VKDevice& device, GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count)
{
    VKAdapter& adapter = device.GetAdapter();
    VKInstance& instance = adapter.GetInstance();

    VkSurfaceKHR surface = 0;
    ASSERT_SUCCEEDED(glfwCreateWindowSurface(instance.GetInstance(), window, nullptr, &surface));
    vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> deleter(instance.GetInstance());
    vk::UniqueSurfaceKHR m_surface = vk::UniqueSurfaceKHR(surface, deleter);

    auto surface_formats = adapter.GetPhysicalDevice().getSurfaceFormatsKHR(m_surface.get());
    ASSERT(!surface_formats.empty());

    if (surface_formats.front().format != vk::Format::eUndefined)
        m_swapchain_color_format = surface_formats.front().format;

    vk::ColorSpaceKHR color_space = surface_formats.front().colorSpace;

    vk::SurfaceCapabilitiesKHR surface_capabilities = {};
    ASSERT_SUCCEEDED(adapter.GetPhysicalDevice().getSurfaceCapabilitiesKHR(m_surface.get(), &surface_capabilities));

    ASSERT(surface_capabilities.currentExtent.width == width);
    ASSERT(surface_capabilities.currentExtent.height == height);

    vk::Bool32 is_supported_surface = VK_FALSE;
    adapter.GetPhysicalDevice().getSurfaceSupportKHR(device.GetQueueFamilyIndex(), m_surface.get(), &is_supported_surface);
    ASSERT(is_supported_surface);

    vk::SwapchainCreateInfoKHR swap_chain_create_info = {};
    swap_chain_create_info.surface = m_surface.get();
    swap_chain_create_info.minImageCount = frame_count;
    swap_chain_create_info.imageFormat = m_swapchain_color_format;
    swap_chain_create_info.imageColorSpace = color_space;
    swap_chain_create_info.imageExtent = surface_capabilities.currentExtent;
    swap_chain_create_info.imageArrayLayers = 1;
    swap_chain_create_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
    swap_chain_create_info.imageSharingMode = vk::SharingMode::eExclusive;
    swap_chain_create_info.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    swap_chain_create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    if (CurState::Instance().vsync)
        swap_chain_create_info.presentMode = vk::PresentModeKHR::eFifo;
    else
        swap_chain_create_info.presentMode = vk::PresentModeKHR::eMailbox;
    swap_chain_create_info.clipped = true;

    m_swapchain = device.GetDevice().createSwapchainKHRUnique(swap_chain_create_info);

    std::vector<vk::Image> m_images = device.GetDevice().getSwapchainImagesKHR(m_swapchain.get());

    for (size_t i = 0; i < frame_count; ++i)
    {
        VKResource::Ptr res = std::make_shared<VKResource>(*this);
        res->image.res = vk::UniqueImage(m_images[i]);
        res->image.format = m_swapchain_color_format;
        res->image.size = { 1u * width, 1u * height };
        res->res_type = VKResource::Type::kImage;
        m_back_buffers.emplace_back(res);
    }
}

void VKSwapchain::QueryOnDelete(VKResource res)
{
}

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#include <GLFW/glfw3.h>
#include <vulkan/vk_sdk_platform.h>
#include "VKContext.h"
#include <Program/VKProgramApi.h>

VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugReportCallback(
    VkDebugReportFlagsEXT       flags,
    VkDebugReportObjectTypeEXT  objectType,
    uint64_t                    object,
    size_t                      location,
    int32_t                     messageCode,
    const char*                 pLayerPrefix,
    const char*                 pMessage,
    void*                       pUserData)
{
    printf("%s\n", pMessage);
    return VK_FALSE;
}

VKContext::VKContext(GLFWwindow* window, int width, int height)
    : Context(window, width, height)
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    const char* pInstExt[] = {
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };

    const char* pInstLayers[] = {
        "VK_LAYER_LUNARG_standard_validation"
    };

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = _countof(pInstExt);
    createInfo.ppEnabledExtensionNames = pInstExt;
    createInfo.enabledLayerCount = _countof(pInstLayers);
    createInfo.ppEnabledLayerNames = pInstLayers;

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);


    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    assert(layerCount != 0, "Failed to find any layer in your system.");

    std::vector<VkLayerProperties> layersAvailable(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layersAvailable.data());

    bool foundValidation = false;
    for (int i = 0; i < layerCount; ++i) {
        if (strcmp(layersAvailable[i].layerName, "VK_LAYER_LUNARG_standard_validation") == 0) {
            foundValidation = true;
        }
    }

#if 1
    VkDebugReportCallbackCreateInfoEXT callbackCreateInfo;
    callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    callbackCreateInfo.pNext = NULL;
    callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
        VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    callbackCreateInfo.pfnCallback = &MyDebugReportCallback;
    callbackCreateInfo.pUserData = NULL;

    PFN_vkCreateDebugReportCallbackEXT my_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT");
    VkDebugReportCallbackEXT callback;
    auto res2 = my_vkCreateDebugReportCallbackEXT(m_instance, &callbackCreateInfo, NULL, &callback);
#endif

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
            deviceFeatures.geometryShader)
            physicalDevice = device;
    }

    auto device = physicalDevice;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int graphicsFamily = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT &&  queueFamily.queueFlags  & VK_QUEUE_COMPUTE_BIT)
        {
            break;
            int b = 0;
        }
        ++graphicsFamily;
    }
    presentQueueFamily = graphicsFamily;

    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsFamily;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo2 = {};
    createInfo2.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo2.pQueueCreateInfos = &queueCreateInfo;
    createInfo2.queueCreateInfoCount = 1;
    createInfo2.pEnabledFeatures = &deviceFeatures;

    const char* pDevExt[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    createInfo2.enabledExtensionCount = _countof(pDevExt);
    createInfo2.ppEnabledExtensionNames = pDevExt;

    if (vkCreateDevice(physicalDevice, &createInfo2, nullptr, &m_device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
    vkGetDeviceQueue(m_device, graphicsFamily, 0, &m_queue);

    auto ptr = vkCreateSwapchainKHR;

    glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface);

    {
        VkBool32 is = false;
        auto resdfg = vkGetPhysicalDeviceSurfaceSupportKHR(device, graphicsFamily, m_surface, &is);
        int b = 0;
    }

    VkExtent2D surfaceResolution;
    {
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface,
            &formatCount, NULL);
        std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface,
            &formatCount, surfaceFormats.data());

        // If the format list includes just one entry of VK_FORMAT_UNDEFINED, the surface has
        // no preferred format. Otherwise, at least one supported format will be returned.
        VkFormat colorFormat;
        if (formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
            colorFormat = VK_FORMAT_B8G8R8_UNORM;
        }
        else {
            colorFormat = surfaceFormats[0].format;
        }
        VkColorSpaceKHR colorSpace;
        colorSpace = surfaceFormats[0].colorSpace;


        VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface,
            &surfaceCapabilities);

        // we are effectively looking for double-buffering:
        // if surfaceCapabilities.maxImageCount == 0 there is actually no limit on the number of images! 
        uint32_t desiredImageCount = 2;
        if (desiredImageCount < surfaceCapabilities.minImageCount) {
            desiredImageCount = surfaceCapabilities.minImageCount;
        }
        else if (surfaceCapabilities.maxImageCount != 0 &&
            desiredImageCount > surfaceCapabilities.maxImageCount) {
            desiredImageCount = surfaceCapabilities.maxImageCount;
        }

        surfaceResolution = surfaceCapabilities.currentExtent;
        if (surfaceResolution.width == -1) {
            surfaceResolution.width = width;
            surfaceResolution.height = height;
        }
        else {
            width = surfaceResolution.width;
            height = surfaceResolution.height;
        }

        VkSurfaceTransformFlagBitsKHR preTransform = surfaceCapabilities.currentTransform;
        if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
            preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }



        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface,
            &presentModeCount, NULL);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface,
            &presentModeCount, presentModes.data());

        VkPresentModeKHR presentationMode = VK_PRESENT_MODE_FIFO_KHR;   // always supported.
        for (uint32_t i = 0; i < presentModeCount; ++i) {
            if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                presentationMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }
    }




    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = m_surface;
    swapChainCreateInfo.minImageCount = FrameCount;
    swapChainCreateInfo.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    swapChainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapChainCreateInfo.imageExtent = surfaceResolution;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapChainCreateInfo.clipped = true;

    VkResult res = vkCreateSwapchainKHR(m_device, &swapChainCreateInfo, nullptr, &m_swapchain);

    uint32_t NumSwapChainImages = 0;
    res = vkGetSwapchainImagesKHR(m_device, m_swapchain, &NumSwapChainImages, nullptr);

    m_images.resize(NumSwapChainImages);
    m_cmd_bufs.resize(NumSwapChainImages);
    res = vkGetSwapchainImagesKHR(m_device, m_swapchain, &NumSwapChainImages, m_images.data());

    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = graphicsFamily;
    res = vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_cmd_pool);


    VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
    cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocInfo.commandPool = m_cmd_pool;
    cmdBufAllocInfo.commandBufferCount = m_cmd_bufs.size();
    cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    res = vkAllocateCommandBuffers(m_device, &cmdBufAllocInfo, m_cmd_bufs.data());

    m_image_views.resize(m_images.size());
    for (size_t i = 0; i < m_images.size(); ++i)
    {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_images[i];
        createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(m_device, &createInfo, nullptr, &m_image_views[i]);
    }

    int b = 0;


    VkSemaphoreCreateInfo createInfoSemaphore = {};
    createInfoSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    vkCreateSemaphore(m_device, &createInfoSemaphore, nullptr, &imageAvailableSemaphore);
    vkCreateSemaphore(m_device, &createInfoSemaphore, nullptr, &renderingFinishedSemaphore);



    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;


    VkClearColorValue clearColor = { 164.0f / 256.0f, 30.0f / 256.0f, 34.0f / 256.0f, 0.0f };
    VkClearValue clearValue = {};
    clearValue.color = clearColor;

    VkImageSubresourceRange imageRange = {};
    imageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageRange.levelCount = 1;
    imageRange.layerCount = 1;

    for (uint32_t i = 0; i < FrameCount; ++i)
    {
        VkResult res = vkBeginCommandBuffer(m_cmd_bufs[i], &beginInfo);

        VkImageMemoryBarrier presentToClearBarrier = {};
        presentToClearBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        presentToClearBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        presentToClearBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        presentToClearBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        presentToClearBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        presentToClearBarrier.srcQueueFamilyIndex = presentQueueFamily;
        presentToClearBarrier.dstQueueFamilyIndex = presentQueueFamily;
        presentToClearBarrier.image = m_images[i];
        presentToClearBarrier.subresourceRange = imageRange;

        // Change layout of image to be optimal for presenting
        VkImageMemoryBarrier clearToPresentBarrier = {};
        clearToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        clearToPresentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        clearToPresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        clearToPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        clearToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        clearToPresentBarrier.srcQueueFamilyIndex = presentQueueFamily;
        clearToPresentBarrier.dstQueueFamilyIndex = presentQueueFamily;
        clearToPresentBarrier.image = m_images[i];
        clearToPresentBarrier.subresourceRange = imageRange;


        vkCmdPipelineBarrier(m_cmd_bufs[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentToClearBarrier);

        vkCmdClearColorImage(m_cmd_bufs[i], m_images[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &imageRange);


        vkCmdPipelineBarrier(m_cmd_bufs[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &clearToPresentBarrier);

        res = vkEndCommandBuffer(m_cmd_bufs[i]);
    }
}

std::unique_ptr<ProgramApi> VKContext::CreateProgram()
{
    return std::make_unique<VKProgramApi>(*this);
}

Resource::Ptr VKContext::CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth, int mip_levels)
{
    return Resource::Ptr();
}

Resource::Ptr VKContext::CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride)
{
    return Resource::Ptr();
}

void VKContext::UpdateSubresource(const Resource::Ptr & ires, uint32_t DstSubresource, const void * pSrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch)
{
}

void VKContext::SetViewport(float width, float height)
{
}

void VKContext::SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
}

void VKContext::IASetIndexBuffer(Resource::Ptr res, uint32_t SizeInBytes, DXGI_FORMAT Format)
{
}

void VKContext::IASetVertexBuffer(uint32_t slot, Resource::Ptr res, uint32_t SizeInBytes, uint32_t Stride)
{
}

void VKContext::BeginEvent(LPCWSTR Name)
{
}

void VKContext::EndEvent()
{
}

void VKContext::DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation)
{
}

void VKContext::Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
{
}

Resource::Ptr VKContext::GetBackBuffer()
{
    return Resource::Ptr();
}

void VKContext::Present(const Resource::Ptr & ires)
{
    VkResult res = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, imageAvailableSemaphore, nullptr, &m_frame_index);


    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_cmd_bufs[m_frame_index];
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
    VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    submitInfo.pWaitDstStageMask = &waitDstStageMask;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderingFinishedSemaphore;

     res = vkQueueSubmit(m_queue, 1, &submitInfo, nullptr);


    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.pImageIndices = &m_frame_index;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderingFinishedSemaphore;

    res = vkQueuePresentKHR(m_queue, &presentInfo);
}

void VKContext::ResizeBackBuffer(int width, int height)
{
}

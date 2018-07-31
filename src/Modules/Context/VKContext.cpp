#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#include <GLFW/glfw3.h>
#include <vulkan/vk_sdk_platform.h>
#include "VKContext.h"
#include "VKResource.h"
#include <Program/VKProgramApi.h>
#include <Geometry/IABuffer.h>
#include <Texture/TextureLoader.h>
#include <glm/glm.hpp>
#include <gli/gli.hpp>

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
#if !defined(_DEBUG)
    return VK_FALSE;
#endif

    std::string msg(pMessage);
    if (msg.find("is being used in draw but has not been updated"))
        return VK_FALSE;
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
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };

    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    assert(layerCount != 0, "Failed to find any layer in your system.");

    std::vector<VkLayerProperties> layersAvailable(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layersAvailable.data());

    std::vector<const char*> pInstLayers;

    for (int i = 0; i < layerCount; ++i) {
        if (strcmp(layersAvailable[i].layerName, "VK_LAYER_LUNARG_standard_validation") == 0) {
            pInstLayers.push_back("VK_LAYER_LUNARG_standard_validation");
        }
    }

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = _countof(pInstExt);
    createInfo.ppEnabledExtensionNames = pInstExt;
    createInfo.enabledLayerCount = pInstLayers.size();
    createInfo.ppEnabledLayerNames = pInstLayers.data();

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);

#if defined(_DEBUG) || 1
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
    m_physical_device = device;
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
    deviceFeatures.textureCompressionBC = true;
    deviceFeatures.vertexPipelineStoresAndAtomics = true;
    deviceFeatures.samplerAnisotropy = true;
    deviceFeatures.fragmentStoresAndAtomics = true;
    
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
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        VkResult res = vkBeginCommandBuffer(m_cmd_bufs[i], &beginInfo);

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

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(m_device, &fenceCreateInfo, NULL, &renderFence);

    for (size_t i = 0; i < FrameCount; ++i)
    {
        descriptor_pool[i].reset(new VKDescriptorPool(*this));
    }
}

std::unique_ptr<ProgramApi> VKContext::CreateProgram()
{
    return std::make_unique<VKProgramApi>(*this);
}

VkFormat VKContext::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physical_device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

Resource::Ptr VKContext::CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth, int mip_levels)
{
    VKResource::Ptr res = std::make_shared<VKResource>();
    res->res_type = VKResource::Type::kImage;

    if (bind_flag & BindFlag::kDsv &&format == DXGI_FORMAT_R32_TYPELESS)
        format = DXGI_FORMAT_D32_FLOAT;

    VkFormat vk_format = static_cast<VkFormat>(gli::dx().find(gli::dx::D3DFMT_DX10, static_cast<gli::dx::dxgi_format_dds>(format)));
    if (vk_format == VK_FORMAT_D24_UNORM_S8_UINT)
        vk_format = VK_FORMAT_D32_SFLOAT_S8_UINT;

    auto createImage = [this](int width, int height, int depth, int mip_levels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
        VkImage& image, VkDeviceMemory& imageMemory, uint32_t& size)
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mip_levels;
        imageInfo.arrayLayers = depth;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (depth == 6)
            imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        if (vkCreateImage(m_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(m_device, image, imageMemory, 0);

        size = allocInfo.allocationSize;
    };
    
    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (bind_flag & BindFlag::kDsv)
        usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (bind_flag & BindFlag::kSrv)
        usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (bind_flag & BindFlag::kRtv)
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (bind_flag & BindFlag::kUav)
        usage |= VK_IMAGE_USAGE_STORAGE_BIT;

    uint32_t tmp = 0;
    createImage(
        width,
        height,
        depth,
        mip_levels,
        vk_format,
        VK_IMAGE_TILING_OPTIMAL,
        usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        res->image.res,
        res->image.memory,
        tmp
    );

    res->image.size.height = height;
    res->image.size.width = width;
    res->image.format = vk_format;
    res->image.level_count = mip_levels;

    return res;
}

uint32_t VKContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physical_device, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

Resource::Ptr VKContext::CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride)
{
    if (buffer_size == 0)
        return VKResource::Ptr();

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = buffer_size;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (bind_flag & BindFlag::kVbv)
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    else if (bind_flag & BindFlag::kIbv)
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    else if (bind_flag & BindFlag::kCbv)
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    else if (bind_flag & BindFlag::kSrv)
        bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    else
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VKResource::Ptr res = std::make_shared<VKResource>();
    res->res_type = VKResource::Type::kBuffer;

    vkCreateBuffer(m_device, &bufferInfo, nullptr, &res->buffer.res);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, res->buffer.res, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);


    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &res->buffer.memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(m_device, res->buffer.res, res->buffer.memory, 0);
    res->buffer.size = buffer_size;

    return res;
}

Resource::Ptr VKContext::CreateSampler(const SamplerDesc & desc)
{
    VKResource::Ptr res = std::make_shared<VKResource>();

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = D3D12_FLOAT32_MAX;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &res->sampler.res) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    res->res_type = VKResource::Type::kSampler;
    return res;
}

static bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void VKContext::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    if (oldLayout == newLayout)
        return;
    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.oldLayout = oldLayout;
    imageMemoryBarrier.newLayout = newLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = image;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format)) {
            imageMemoryBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else {
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;


    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch (oldLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        imageMemoryBarrier.srcAccessMask = 0;
        break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image is a transfer source 
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newLayout)
    {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if (imageMemoryBarrier.srcAccessMask == 0)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    vkCmdPipelineBarrier(
        m_cmd_bufs[m_frame_index],
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier
    );
};

void VKContext::UpdateSubresource(const Resource::Ptr & ires, uint32_t DstSubresource, const void * pSrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch)
{
    if (!ires)
        return;
    auto res = std::static_pointer_cast<VKResource>(ires);

    if (res->res_type == VKResource::Type::kBuffer)
    {
        void* data;
        vkMapMemory(m_device, res->buffer.memory, 0, res->buffer.size, 0, &data);
        memcpy(data, pSrcData, (size_t)res->buffer.size);
        vkUnmapMemory(m_device, res->buffer.memory);
    }
    else if (res->res_type == VKResource::Type::kImage)
    {
        auto staging = res->GetUploadResource(DstSubresource);
        if (!staging || staging->res_type == VKResource::Type::kUnknown)
            staging = std::static_pointer_cast<VKResource>(CreateBuffer(0, SrcDepthPitch, 0));
        UpdateSubresource(staging, 0, pSrcData, SrcRowPitch, SrcDepthPitch);

        // Setup buffer copy regions for each mip level
        std::vector<VkBufferImageCopy> bufferCopyRegions;
        uint32_t offset = 0;

        VkBufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel = DstSubresource;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(res->image.size.width >> DstSubresource);
        bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(res->image.size.height >> DstSubresource);
        bufferCopyRegion.imageExtent.depth = 1;

        bufferCopyRegions.push_back(bufferCopyRegion);

        // The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
        VkImageSubresourceRange subresourceRange = {};
        // Image only contains color data
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        // Start at first mip level
        subresourceRange.baseMipLevel = DstSubresource;
        // We will transition on all mip levels
        subresourceRange.levelCount = 1;
        // The 2D texture only has one layer
        subresourceRange.layerCount = 1;

        // Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
        VkImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.image = res->image.res;
        imageMemoryBarrier.subresourceRange = subresourceRange;
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        // Insert a memory dependency at the proper pipeline stages that will execute the image layout transition 
        // Source pipeline stage is host write/read exection (VK_PIPELINE_STAGE_HOST_BIT)
        // Destination pipeline stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
        vkCmdPipelineBarrier(
            m_cmd_bufs[m_frame_index],
            VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);

        // Copy mip levels from staging buffer
        vkCmdCopyBufferToImage(
            m_cmd_bufs[m_frame_index],
            staging->buffer.res,
            res->image.res,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            static_cast<uint32_t>(bufferCopyRegions.size()),
            bufferCopyRegions.data());

        // Once the data has been uploaded we transfer to the texture image to the shader read layout, so it can be sampled from
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Insert a memory dependency at the proper pipeline stages that will execute the image layout transition 
        // Source pipeline stage stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
        // Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
        vkCmdPipelineBarrier(
            m_cmd_bufs[m_frame_index],
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);

        // Store current layout for later reuse
        res->image.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
}

void VKContext::SetViewport(float width, float height)
{
    VkViewport viewport{};
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1.0;
    vkCmdSetViewport(m_cmd_bufs[m_frame_index], 0, 1, &viewport);

    SetScissorRect(0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height));
}

void VKContext::SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    VkRect2D rect2D{};
    rect2D.extent.width = right;
    rect2D.extent.height = bottom;
    rect2D.offset.x = left;
    rect2D.offset.y = top;
    vkCmdSetScissor(m_cmd_bufs[m_frame_index], 0, 1, &rect2D);
}

void VKContext::IASetIndexBuffer(Resource::Ptr ires, uint32_t SizeInBytes, DXGI_FORMAT Format)
{
    VKResource::Ptr res = std::static_pointer_cast<VKResource>(ires);
    auto format = static_cast<VkFormat>(gli::dx().find(gli::dx::D3DFMT_DX10, static_cast<gli::dx::dxgi_format_dds>(Format)));
    VkIndexType index_type = {};
    switch (format)
    {
    case VK_FORMAT_R16_UINT:
        index_type = VK_INDEX_TYPE_UINT16;
        break;
    case VK_FORMAT_R32_UINT:
        index_type = VK_INDEX_TYPE_UINT32;
        break;
    default:
        break;
    }

    vkCmdBindIndexBuffer(m_cmd_bufs[m_frame_index], res->buffer.res, 0, index_type);
}

void VKContext::IASetVertexBuffer(uint32_t slot, Resource::Ptr ires, uint32_t SizeInBytes, uint32_t Stride)
{
    VKResource::Ptr res = std::static_pointer_cast<VKResource>(ires);
    VkBuffer vertexBuffers[] = { res->buffer.res };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(m_cmd_bufs[m_frame_index], slot, 1, vertexBuffers, offsets);
}

void VKContext::BeginEvent(LPCWSTR Name)
{
}

void VKContext::EndEvent()
{
}

void VKContext::DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation)
{
    m_current_program->ApplyBindings();
    vkCmdDrawIndexed(m_cmd_bufs[m_frame_index], IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

void VKContext::Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
{
}

Resource::Ptr VKContext::GetBackBuffer()
{
    VKResource::Ptr res = std::make_shared<VKResource>();
    res->image.res = m_images[m_frame_index];
    res->image.format = VK_FORMAT_R8G8B8A8_UNORM;
    res->image.size = { 1u * m_width, 1u * m_height };
    res->res_type = VKResource::Type::kImage;
    return res;
}

void VKContext::Present(const Resource::Ptr & ires)
{
    m_current_program->OnPresent();

    vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, imageAvailableSemaphore, nullptr, &m_frame_index);

    transitionImageLayout(m_images[m_frame_index], {}, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    auto res = vkEndCommandBuffer(m_cmd_bufs[m_frame_index]);

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

    res = vkQueueSubmit(m_queue, 1, &submitInfo, renderFence);

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.pImageIndices = &m_frame_index;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderingFinishedSemaphore;

    res = vkQueuePresentKHR(m_queue, &presentInfo);


    vkWaitForFences(m_device, 1, &renderFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &renderFence);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    res = vkBeginCommandBuffer(m_cmd_bufs[m_frame_index], &beginInfo);

    descriptor_pool[m_frame_index]->OnFrameBegin();
}

void VKContext::ResizeBackBuffer(int width, int height)
{
}

void VKContext::UseProgram(VKProgramApi & program_api)
{
    m_current_program = &program_api;
}

VKDescriptorPool& VKContext::GetDescriptorPool()
{
    return *descriptor_pool[m_frame_index];
}

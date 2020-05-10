#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKContext.h"
#include <Resource/VKResource.h>
#include <Program/VKProgramApi.h>
#include <Geometry/IABuffer.h>
#include <Texture/TextureLoader.h>
#include <glm/glm.hpp>
#include <gli/gli.hpp>
#include <Utilities/VKUtility.h>
#include <Utilities/State.h>
#include <sstream>

vk::IndexType GetVkIndexType(gli::format Format)
{
    vk::Format format = static_cast<vk::Format>(Format);
    switch (format)
    {
    case vk::Format::eR16Uint:
        return vk::IndexType::eUint16;
    case vk::Format::eR32Uint:
        return vk::IndexType::eUint32;
    default:
        assert(false);
        return {};
    }
}

class DebugReportListener
{
public:
    DebugReportListener(const vk::UniqueInstance& instance)
    {
        if (!enabled)
            return;
        vk::DebugReportCallbackCreateInfoEXT callback_create_info = {};
        callback_create_info.flags = vk::DebugReportFlagBitsEXT::eWarning |
                                     vk::DebugReportFlagBitsEXT::ePerformanceWarning |
                                     vk::DebugReportFlagBitsEXT::eError |
                                     vk::DebugReportFlagBitsEXT::eDebug;
        callback_create_info.pfnCallback = &DebugReportCallback;
        m_callback = instance->createDebugReportCallbackEXT(callback_create_info);
    }

    static constexpr bool enabled = true;

private:
    static bool SkipIt(VkDebugReportObjectTypeEXT object_type, const std::string& message)
    {
        switch (object_type)
        {
        case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT:
            return true;
        default:
            return false;
        }
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
        VkDebugReportFlagsEXT       flags,
        VkDebugReportObjectTypeEXT  objectType,
        uint64_t                    object,
        size_t                      location,
        int32_t                     messageCode,
        const char*                 pLayerPrefix,
        const char*                 pMessage,
        void*                       pUserData)
    {
        if (SkipIt(objectType, pMessage))
            return VK_FALSE;
        static size_t error_count = 0;
#ifdef _WIN32
        if (++error_count <= 1000)
        {
            std::stringstream buf;
            buf << pLayerPrefix << " " << to_string(static_cast<vk::DebugReportFlagBitsEXT>(flags)) << " " << pMessage << std::endl;
            OutputDebugStringA(buf.str().c_str());
        }
#endif
        return VK_FALSE;
    }

    vk::DebugReportCallbackEXT m_callback;
};

void VKContext::CreateInstance()
{
    auto layers = vk::enumerateInstanceLayerProperties();

    std::set<std::string> req_layers;
    if (DebugReportListener::enabled)
        req_layers.insert("VK_LAYER_LUNARG_standard_validation");
    std::vector<const char*> found_layers;
    for (const auto& layer : layers)
    {
        if (req_layers.count(layer.layerName))
            found_layers.push_back(layer.layerName);
    }

    auto extensions = vk::enumerateInstanceExtensionProperties();

    std::set<std::string> req_extension = {
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
    #if defined(VK_USE_PLATFORM_WIN32_KHR)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    #elif defined(VK_USE_PLATFORM_XLIB_KHR)
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
    #elif defined(VK_USE_PLATFORM_XCB_KHR)
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
    #endif
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    };
    std::vector<const char*> found_extension;
    for (const auto& extension : extensions)
    {
        if (req_extension.count(extension.extensionName))
            found_extension.push_back(extension.extensionName);
    }

    vk::ApplicationInfo app_info = {};
    app_info.apiVersion = VK_API_VERSION_1_0;

    vk::InstanceCreateInfo create_info;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledLayerCount = found_layers.size();
    create_info.ppEnabledLayerNames = found_layers.data();
    create_info.enabledExtensionCount = found_extension.size();
    create_info.ppEnabledExtensionNames = found_extension.data();

    m_instance = vk::createInstanceUnique(create_info);

    DebugReportListener{ m_instance };
}

void VKContext::SelectPhysicalDevice()
{
    auto devices = m_instance->enumeratePhysicalDevices();

    uint32_t gpu_index = 0;
    for (const auto& device : devices)
    {
        vk::PhysicalDeviceProperties device_properties = device.getProperties();

        if (device_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ||
            device_properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
        {
            if (CurState::Instance().required_gpu_index != -1 && gpu_index++ != CurState::Instance().required_gpu_index)
                continue;
            m_physical_device = device;
            CurState::Instance().gpu_name = device_properties.deviceName;
            break;
        }
    }
}

void VKContext::SelectQueueFamilyIndex()
{
    auto queue_families = m_physical_device.getQueueFamilyProperties();

    m_queue_family_index = -1;
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
}

void VKContext::CreateDevice()
{
    auto extensions = m_physical_device.enumerateDeviceExtensionProperties();
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

    m_device = m_physical_device.createDeviceUnique(device_create_info);
}

void VKContext::CreateSwapchain(int width, int height)
{
    auto surface_formats = m_physical_device.getSurfaceFormatsKHR(m_surface.get());
    ASSERT(!surface_formats.empty());

    if (surface_formats.front().format != vk::Format::eUndefined)
        m_swapchain_color_format = surface_formats.front().format;

    vk::ColorSpaceKHR color_space = surface_formats.front().colorSpace;

    vk::SurfaceCapabilitiesKHR surface_capabilities = {};
    ASSERT_SUCCEEDED(m_physical_device.getSurfaceCapabilitiesKHR(m_surface.get(), &surface_capabilities));

    ASSERT(surface_capabilities.currentExtent.width == width);
    ASSERT(surface_capabilities.currentExtent.height == height);

    vk::Bool32 is_supported_surface = VK_FALSE;
    m_physical_device.getSurfaceSupportKHR(m_queue_family_index, m_surface.get(), &is_supported_surface);
    ASSERT(is_supported_surface);

    vk::SwapchainCreateInfoKHR swap_chain_create_info = {};
    swap_chain_create_info.surface = m_surface.get();
    swap_chain_create_info.minImageCount = FrameCount;
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

    m_swapchain = m_device->createSwapchainKHRUnique(swap_chain_create_info);
}

VKContext::VKContext(GLFWwindow* window)
    : Context(window)
{
    CreateInstance();
    SelectPhysicalDevice();
    SelectQueueFamilyIndex();
    CreateDevice();

    m_queue = m_device->getQueue(m_queue_family_index, 0);

    VkSurfaceKHR surface;
    ASSERT_SUCCEEDED(glfwCreateWindowSurface(m_instance.get(), window, nullptr, &surface));
    vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> deleter(m_instance.get());
    m_surface = vk::UniqueSurfaceKHR(surface, deleter);

    CreateSwapchain(m_width, m_height);

    m_images = m_device->getSwapchainImagesKHR(m_swapchain.get());

    vk::CommandPoolCreateInfo cmd_pool_create_info = {};
    cmd_pool_create_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    cmd_pool_create_info.queueFamilyIndex = m_queue_family_index;
    m_cmd_pool = m_device->createCommandPoolUnique(cmd_pool_create_info);

    m_cmd_bufs.resize(m_images.size());
    vk::CommandBufferAllocateInfo cmd_buf_alloc_info = {};
    cmd_buf_alloc_info.commandPool = m_cmd_pool.get();
    cmd_buf_alloc_info.commandBufferCount = m_cmd_bufs.size();
    cmd_buf_alloc_info.level = vk::CommandBufferLevel::ePrimary;
    m_cmd_bufs = m_device->allocateCommandBuffersUnique(cmd_buf_alloc_info);

    vk::SemaphoreCreateInfo semaphore_create_info = {};

    m_image_available_semaphore = m_device->createSemaphoreUnique(semaphore_create_info);
    m_rendering_finished_semaphore = m_device->createSemaphoreUnique(semaphore_create_info);

    vk::FenceCreateInfo fence_create_info = {};
    m_fence = m_device->createFenceUnique(fence_create_info);

    ASSERT(m_images.size() == FrameCount);
    for (size_t i = 0; i < FrameCount; ++i)
    {
        descriptor_pool[i].reset(new VKDescriptorPool(*this));
    }

    OpenCommandBuffer();

    for (size_t i = 0; i < FrameCount; ++i)
    {
        VKResource::Ptr res = std::make_shared<VKResource>(*this);
        res->image.res = vk::UniqueImage(m_images[i]);
        res->image.format = m_swapchain_color_format;
        res->image.size = vk::Extent2D(1u * m_width, 1u * m_height);
        res->res_type = VKResource::Type::kImage;
        m_back_buffers[i] = res;
    }
}

std::unique_ptr<ProgramApi> VKContext::CreateProgram()
{
    auto res = std::make_unique<VKProgramApi>(*this);
    m_created_program.push_back(*res.get());
    return res;
}

vk::Format VKContext::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
    for (const vk::Format& format : candidates)
    {
        vk::FormatProperties props = m_physical_device.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
            return format;
        else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
            return format;
    }

    throw std::runtime_error("failed to find supported format!");
}

Resource::Ptr VKContext::CreateTexture(uint32_t bind_flag, gli::format format, uint32_t msaa_count, int width, int height, int depth, int mip_levels)
{
    VKResource::Ptr res = std::make_shared<VKResource>(*this);
    res->res_type = VKResource::Type::kImage;

    vk::Format vk_format = static_cast<vk::Format>(format);
    if (vk_format == vk::Format::eD24UnormS8Uint)
        vk_format = vk::Format::eD32SfloatS8Uint;

    auto createImage = [this, msaa_count](int width, int height, int depth, int mip_levels, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
        vk::UniqueImage& image, vk::UniqueDeviceMemory& imageMemory, uint32_t& size)
    {
        vk::ImageCreateInfo imageInfo = {};
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mip_levels;
        imageInfo.arrayLayers = depth;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;
        imageInfo.usage = usage;
        imageInfo.samples = static_cast<vk::SampleCountFlagBits>(msaa_count);
        imageInfo.sharingMode = vk::SharingMode::eExclusive;

        if (depth % 6 == 0)
            imageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;

        image = m_device->createImageUnique(imageInfo);

        vk::MemoryRequirements memRequirements;
        m_device->getImageMemoryRequirements(image.get(), &memRequirements);

        vk::MemoryAllocateInfo allocInfo = {};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        imageMemory = m_device->allocateMemoryUnique(allocInfo);
        m_device->bindImageMemory(image.get(), imageMemory.get(), 0);

        size = allocInfo.allocationSize;
    };
    
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eTransferDst;
    if (bind_flag & BindFlag::kDsv)
        usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    if (bind_flag & BindFlag::kSrv)
        usage |= vk::ImageUsageFlagBits::eSampled;
    if (bind_flag & BindFlag::kRtv)
        usage |= vk::ImageUsageFlagBits::eColorAttachment;
    if (bind_flag & BindFlag::kUav)
        usage |= vk::ImageUsageFlagBits::eStorage;

    uint32_t tmp = 0;
    createImage(
        width,
        height,
        depth,
        mip_levels,
        vk_format,
        vk::ImageTiling::eOptimal,
        usage,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        res->image.res,
        res->image.memory,
        tmp
    );

    res->image.size.height = height;
    res->image.size.width = width;
    res->image.format = vk_format;
    res->image.level_count = mip_levels;
    res->image.msaa_count = msaa_count;
    res->image.array_layers = depth;

    return res;
}

uint32_t VKContext::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memProperties;
    m_physical_device.getMemoryProperties(&memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

Resource::Ptr VKContext::CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride)
{
    if (buffer_size == 0)
        return VKResource::Ptr();

    vk::BufferCreateInfo bufferInfo = {};
    bufferInfo.size = buffer_size;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    if (bind_flag & BindFlag::kVbv)
        bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
    else if (bind_flag & BindFlag::kIbv)
        bufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;
    else if (bind_flag & BindFlag::kCbv)
        bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
    else if (bind_flag & BindFlag::kSrv)
        bufferInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer;
    else
        bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

    VKResource::Ptr res = std::make_shared<VKResource>(*this);
    res->res_type = VKResource::Type::kBuffer;

    res->buffer.res = m_device->createBufferUnique(bufferInfo);

    vk::MemoryRequirements memRequirements;
    m_device->getBufferMemoryRequirements(res->buffer.res.get(), &memRequirements);

    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    res->buffer.memory = m_device->allocateMemoryUnique(allocInfo);

    m_device->bindBufferMemory(res->buffer.res.get(), res->buffer.memory.get(), 0);
    res->buffer.size = buffer_size;

    return res;
}

Resource::Ptr VKContext::CreateSampler(const SamplerDesc & desc)
{
    VKResource::Ptr res = std::make_shared<VKResource>(*this);

    vk::SamplerCreateInfo samplerInfo = {};
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = std::numeric_limits<float>::max();

    /*switch (desc.filter)
    {
    case SamplerFilter::kAnisotropic:
        sampler_desc.Filter = D3D12_FILTER_ANISOTROPIC;
        break;
    case SamplerFilter::kMinMagMipLinear:
        sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        break;
    case SamplerFilter::kComparisonMinMagMipLinear:
        sampler_desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        break;
    }*/

    switch (desc.mode)
    {
    case SamplerTextureAddressMode::kWrap:
        samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
        break;
    case SamplerTextureAddressMode::kClamp:
        samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
        break;
    }

    switch (desc.func)
    {
    case SamplerComparisonFunc::kNever:
        samplerInfo.compareOp = vk::CompareOp::eNever;
        break;
    case SamplerComparisonFunc::kAlways:
        samplerInfo.compareEnable = true;
        samplerInfo.compareOp = vk::CompareOp::eAlways;
        break;
    case SamplerComparisonFunc::kLess:
        samplerInfo.compareEnable = true;
        samplerInfo.compareOp = vk::CompareOp::eLess;
        break;
    }

    res->sampler.res = m_device->createSamplerUnique(samplerInfo);

    res->res_type = VKResource::Type::kSampler;
    return res;
}

vk::ImageAspectFlags VKContext::GetAspectFlags(vk::Format format)
{
    switch (format)
    {
    case vk::Format::eD32SfloatS8Uint:
    case vk::Format::eD24UnormS8Uint:
        return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    case vk::Format::eD32Sfloat:
        return vk::ImageAspectFlagBits::eDepth;
    default:
        return vk::ImageAspectFlagBits::eColor;
    }
}

void VKContext::TransitionImageLayout(VKResource::Image& image, vk::ImageLayout newLayout, const ViewDesc& view_desc)
{
    vk::ImageSubresourceRange range = {};
    range.aspectMask = GetAspectFlags(image.format);
    range.baseMipLevel = view_desc.level;
    if (newLayout == vk::ImageLayout::eColorAttachmentOptimal)
        range.levelCount = 1;
    else if (view_desc.count == -1)
        range.levelCount = image.level_count - view_desc.level;
    else
        range.levelCount = view_desc.count;
    range.baseArrayLayer = 0;
    range.layerCount = image.array_layers;

    std::vector<vk::ImageMemoryBarrier> image_memory_barriers;

    for (uint32_t i = 0; i < range.levelCount; ++i)
    {
        for (uint32_t j = 0; j < range.layerCount; ++j)
        {
            vk::ImageSubresourceRange barrier_range = range;
            barrier_range.baseMipLevel = range.baseMipLevel + i;
            barrier_range.levelCount = 1;
            barrier_range.baseArrayLayer = range.baseArrayLayer + j;
            barrier_range.layerCount = 1;

            if (image.layout[barrier_range] == newLayout)
                continue;

            image_memory_barriers.emplace_back();
            vk::ImageMemoryBarrier& imageMemoryBarrier = image_memory_barriers.back();
            imageMemoryBarrier.oldLayout = image.layout[barrier_range];

            imageMemoryBarrier.newLayout = newLayout;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image = image.res.get();
            imageMemoryBarrier.subresourceRange = barrier_range;

            // Source layouts (old)
            // Source access mask controls actions that have to be finished on the old layout
            // before it will be transitioned to the new layout
            switch (image.layout[barrier_range])
            {
            case vk::ImageLayout::eUndefined:
                // Image layout is undefined (or does not matter)
                // Only valid as initial layout
                // No flags required, listed only for completeness
                imageMemoryBarrier.srcAccessMask = {};
                break;
            case vk::ImageLayout::ePreinitialized:
                // Image is preinitialized
                // Only valid as initial layout for linear images, preserves memory contents
                // Make sure host writes have been finished
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
                break;
            case vk::ImageLayout::eColorAttachmentOptimal:
                // Image is a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
                break;
            case vk::ImageLayout::eDepthAttachmentOptimal:
                // Image is a depth/stencil attachment
                // Make sure any writes to the depth/stencil buffer have been finished
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
                break;
            case vk::ImageLayout::eTransferSrcOptimal:
                // Image is a transfer source 
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
                break;
            case vk::ImageLayout::eTransferDstOptimal:
                // Image is a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                break;

            case vk::ImageLayout::eShaderReadOnlyOptimal:
                // Image is read by a shader
                // Make sure any shader reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
            }

            // Target layouts (new)
            // Destination access mask controls the dependency for the new image layout
            switch (newLayout)
            {
            case vk::ImageLayout::eTransferDstOptimal:
                // Image will be used as a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
                break;

            case vk::ImageLayout::eTransferSrcOptimal:
                // Image will be used as a transfer source
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
                break;

            case vk::ImageLayout::eColorAttachmentOptimal:
                // Image will be used as a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
                break;

            case vk::ImageLayout::eDepthAttachmentOptimal:
                // Image layout will be used as a depth/stencil attachment
                // Make sure any writes to depth/stencil buffer have been finished
                imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
                break;

            case vk::ImageLayout::eShaderReadOnlyOptimal:
                // Image will be read in a shader (sampler, input attachment)
                // Make sure any writes to the image have been finished
                if (!imageMemoryBarrier.srcAccessMask)
                {
                    imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite;
                }
                imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
            }

            image.layout[barrier_range] = newLayout;
        }
    }

    m_cmd_bufs[m_frame_index]->pipelineBarrier(
        vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
        vk::DependencyFlagBits::eByRegion,
        0, nullptr,
        0, nullptr,
        image_memory_barriers.size(), image_memory_barriers.data());
};

void VKContext::UpdateSubresource(const Resource::Ptr & ires, uint32_t DstSubresource, const void * pSrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch)
{
    if (!ires)
        return;
    auto res = std::static_pointer_cast<VKResource>(ires);

    if (res->res_type == VKResource::Type::kBuffer)
    {
        void* data;
        m_device->mapMemory(res->buffer.memory.get(), 0, res->buffer.size, {}, &data);
        memcpy(data, pSrcData, (size_t)res->buffer.size);
        m_device->unmapMemory(res->buffer.memory.get());
    }
    else if (res->res_type == VKResource::Type::kImage)
    {
        auto staging = res->GetUploadResource(DstSubresource);
        if (!staging || staging->res_type == VKResource::Type::kUnknown)
            staging = std::static_pointer_cast<VKResource>(CreateBuffer(0, SrcDepthPitch, 0));
        UpdateSubresource(staging, 0, pSrcData, SrcRowPitch, SrcDepthPitch);

        // Setup buffer copy regions for each mip level
        std::vector<vk::BufferImageCopy> bufferCopyRegions;
        uint32_t offset = 0;

        vk::BufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        bufferCopyRegion.imageSubresource.mipLevel = DstSubresource;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = std::max(1u, static_cast<uint32_t>(res->image.size.width >> DstSubresource));
        bufferCopyRegion.imageExtent.height = std::max(1u, static_cast<uint32_t>(res->image.size.height >> DstSubresource));
        bufferCopyRegion.imageExtent.depth = 1;

        bufferCopyRegions.push_back(bufferCopyRegion);

        // The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
        vk::ImageSubresourceRange subresourceRange = {};
        // Image only contains color data
        subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        // Start at first mip level
        subresourceRange.baseMipLevel = DstSubresource;
        // We will transition on all mip levels
        subresourceRange.levelCount = 1;
        // The 2D texture only has one layer
        subresourceRange.layerCount = 1;

        // Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
        vk::ImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.image = res->image.res.get();
        imageMemoryBarrier.subresourceRange = subresourceRange;
        imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        imageMemoryBarrier.oldLayout = vk::ImageLayout::eUndefined;
        imageMemoryBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;

        // Insert a memory dependency at the proper pipeline stages that will execute the image layout transition 
        // Source pipeline stage is host write/read exection (vk::PipelineStageFlagBits::eHost)
        // Destination pipeline stage is copy command exection (vk::PipelineStageFlagBits::eTransfer)
        m_cmd_bufs[m_frame_index]->pipelineBarrier(
            vk::PipelineStageFlagBits::eHost,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);

        // Copy mip levels from staging buffer
        m_cmd_bufs[m_frame_index]->copyBufferToImage(
            staging->buffer.res.get(),
            res->image.res.get(),
            vk::ImageLayout::eTransferDstOptimal,
            static_cast<uint32_t>(bufferCopyRegions.size()),
            bufferCopyRegions.data());

        // Once the data has been uploaded we transfer to the texture image to the shader read layout, so it can be sampled from
        imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        imageMemoryBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        imageMemoryBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        // Insert a memory dependency at the proper pipeline stages that will execute the image layout transition 
        // Source pipeline stage stage is copy command exection (vk::PipelineStageFlagBits::eTransfer)
        // Destination pipeline stage fragment shader access (vk::PipelineStageFlagBits::eFragmentShader)
        m_cmd_bufs[m_frame_index]->pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            {},
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);

        // Store current layout for later reuse
        res->image.layout[subresourceRange] = vk::ImageLayout::eShaderReadOnlyOptimal;
    }
}

void VKContext::SetViewport(float width, float height)
{
    vk::Viewport viewport{};
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1.0;
    m_cmd_bufs[m_frame_index]->setViewport(0, 1, &viewport);

    SetScissorRect(0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height));
}

void VKContext::SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    vk::Rect2D rect2D{};
    rect2D.extent.width = right;
    rect2D.extent.height = bottom;
    rect2D.offset.x = left;
    rect2D.offset.y = top;
    m_cmd_bufs[m_frame_index]->setScissor(0, 1, &rect2D);
}

Resource::Ptr VKContext::CreateBottomLevelAS(const BufferDesc& vertex)
{
    return CreateBottomLevelAS(vertex, {});
}

Resource::Ptr VKContext::CreateBottomLevelAS(const BufferDesc& vertex, const BufferDesc& index)
{
    VKResource::Ptr res = std::make_shared<VKResource>(*this);
    res->res_type = VKResource::Type::kBottomLevelAS;
    AccelerationStructure& bottomLevelAS = res->bottom_as;

    auto vertex_res = std::static_pointer_cast<VKResource>(vertex.res);
    auto index_res = std::static_pointer_cast<VKResource>(index.res);

    auto vertex_stride = gli::detail::bits_per_pixel(vertex.format) / 8;

    vk::GeometryNV& geometry = bottomLevelAS.geometry;
    geometry.geometryType = vk::GeometryTypeNV::eTriangles;
    geometry.geometry.triangles.vertexData = vertex_res->buffer.res.get();
    geometry.geometry.triangles.vertexOffset = vertex.offset * vertex_stride;
    geometry.geometry.triangles.vertexCount = vertex.count;
    geometry.geometry.triangles.vertexStride = vertex_stride;
    geometry.geometry.triangles.vertexFormat = static_cast<vk::Format>(vertex.format);
    if (index_res)
    {
        auto index_stride = gli::detail::bits_per_pixel(index.format) / 8;
        geometry.geometry.triangles.indexData = index_res->buffer.res.get();
        geometry.geometry.triangles.indexOffset = index.offset * index_stride;
        geometry.geometry.triangles.indexCount = index.count;
        geometry.geometry.triangles.indexType = GetVkIndexType(index.format);
    }
    else
    {
        geometry.geometry.triangles.indexType = vk::IndexType::eNoneNV;
    }

    geometry.geometry.triangles.transformOffset = 0;
    geometry.flags = vk::GeometryFlagBitsNV::eOpaque;

    vk::AccelerationStructureInfoNV accelerationStructureInfo{};
    accelerationStructureInfo.type = vk::AccelerationStructureTypeNV::eBottomLevel;
    accelerationStructureInfo.instanceCount = 0;
    accelerationStructureInfo.geometryCount = 1;
    accelerationStructureInfo.pGeometries = &geometry;

    vk::AccelerationStructureCreateInfoNV accelerationStructureCI{};
    accelerationStructureCI.info = accelerationStructureInfo;
    bottomLevelAS.accelerationStructure = m_device->createAccelerationStructureNVUnique(accelerationStructureCI);

    vk::AccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
    memoryRequirementsInfo.type = vk::AccelerationStructureMemoryRequirementsTypeNV::eObject;
    memoryRequirementsInfo.accelerationStructure = bottomLevelAS.accelerationStructure.get();

    vk::MemoryRequirements2 memoryRequirements2{};
    m_device->getAccelerationStructureMemoryRequirementsNV(&memoryRequirementsInfo, &memoryRequirements2);

    vk::MemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements2.memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    bottomLevelAS.memory = m_device->allocateMemoryUnique(memoryAllocateInfo);

    vk::BindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
    accelerationStructureMemoryInfo.accelerationStructure = bottomLevelAS.accelerationStructure.get();
    accelerationStructureMemoryInfo.memory = bottomLevelAS.memory.get();
    m_device->bindAccelerationStructureMemoryNV(1, &accelerationStructureMemoryInfo);

    m_device->getAccelerationStructureHandleNV(bottomLevelAS.accelerationStructure.get(), sizeof(uint64_t), &bottomLevelAS.handle);

    return res;
}

Resource::Ptr VKContext::CreateTopLevelAS(const std::vector<std::pair<Resource::Ptr, glm::mat4>>& geometry)
{
    VKResource::Ptr res = std::make_shared<VKResource>(*this);
    res->res_type = VKResource::Type::kTopLevelAS;
    AccelerationStructure& topLevelAS = res->top_as;

    vk::AccelerationStructureInfoNV accelerationStructureInfo{};
    accelerationStructureInfo.type = vk::AccelerationStructureTypeNV::eTopLevel;
    accelerationStructureInfo.instanceCount = geometry.size();
    accelerationStructureInfo.geometryCount = 0;

    vk::AccelerationStructureCreateInfoNV accelerationStructureCI{};
    accelerationStructureCI.info = accelerationStructureInfo;
    topLevelAS.accelerationStructure = m_device->createAccelerationStructureNVUnique(accelerationStructureCI);

    vk::AccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
    memoryRequirementsInfo.type = vk::AccelerationStructureMemoryRequirementsTypeNV::eObject;
    memoryRequirementsInfo.accelerationStructure = topLevelAS.accelerationStructure.get();

    vk::MemoryRequirements2 memoryRequirements2{};
    m_device->getAccelerationStructureMemoryRequirementsNV(&memoryRequirementsInfo, &memoryRequirements2);

    vk::MemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements2.memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    topLevelAS.memory = m_device->allocateMemoryUnique(memoryAllocateInfo);

    vk::BindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
    accelerationStructureMemoryInfo.accelerationStructure = topLevelAS.accelerationStructure.get();
    accelerationStructureMemoryInfo.memory = topLevelAS.memory.get();
    ASSERT_SUCCEEDED(m_device->bindAccelerationStructureMemoryNV(1, &accelerationStructureMemoryInfo));
    ASSERT_SUCCEEDED(m_device->getAccelerationStructureHandleNV(topLevelAS.accelerationStructure.get(), sizeof(uint64_t), &topLevelAS.handle));

    memoryRequirementsInfo.type = vk::AccelerationStructureMemoryRequirementsTypeNV::eBuildScratch;

    vk::DeviceSize maximumBlasSize = 0;
    for (auto& mesh : geometry)
    {
        auto res = std::static_pointer_cast<VKResource>(mesh.first);

        memoryRequirementsInfo.type = vk::AccelerationStructureMemoryRequirementsTypeNV::eBuildScratch;
        memoryRequirementsInfo.accelerationStructure = res->bottom_as.accelerationStructure.get();

        vk::MemoryRequirements2 memReqBLAS;
        m_device->getAccelerationStructureMemoryRequirementsNV(&memoryRequirementsInfo, &memReqBLAS);

        maximumBlasSize = std::max(maximumBlasSize, memReqBLAS.memoryRequirements.size);
    }

    vk::MemoryRequirements2 memReqTopLevelAS;
    memoryRequirementsInfo.accelerationStructure = topLevelAS.accelerationStructure.get();
    m_device->getAccelerationStructureMemoryRequirementsNV(&memoryRequirementsInfo, &memReqTopLevelAS);

    const vk::DeviceSize scratchBufferSize = std::max(maximumBlasSize, memReqTopLevelAS.memoryRequirements.size);


    {
        vk::BufferCreateInfo bufferInfo = {};
        bufferInfo.size = scratchBufferSize;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;
        bufferInfo.usage = vk::BufferUsageFlagBits::eRayTracingNV;
        topLevelAS.scratchBuffer = m_device->createBufferUnique(bufferInfo);

        vk::MemoryRequirements memRequirements;
        m_device->getBufferMemoryRequirements(topLevelAS.scratchBuffer.get(), &memRequirements);

        vk::MemoryAllocateInfo allocInfo = {};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

        topLevelAS.scratchmemory = m_device->allocateMemoryUnique(allocInfo);

        m_device->bindBufferMemory(topLevelAS.scratchBuffer.get(), topLevelAS.scratchmemory.get(), 0);
    }

    for (auto& mesh : geometry)
    {
        auto res = std::static_pointer_cast<VKResource>(mesh.first);
        
        vk::AccelerationStructureInfoNV buildInfo{};
        buildInfo.type = vk::AccelerationStructureTypeNV::eBottomLevel;
        buildInfo.instanceCount = 0;
        buildInfo.geometryCount = 1;

        buildInfo.pGeometries = &res->bottom_as.geometry;

        m_cmd_bufs[m_frame_index]->buildAccelerationStructureNV(
            buildInfo,
            {},
            0,
            VK_FALSE,
            res->bottom_as.accelerationStructure.get(),
            {},
            topLevelAS.scratchBuffer.get(),
            0
        );
    }

    vk::MemoryBarrier memoryBarrier = {};
    memoryBarrier.pNext = nullptr;
    memoryBarrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteNV | vk::AccessFlagBits::eAccelerationStructureReadNV;
    memoryBarrier.dstAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteNV | vk::AccessFlagBits::eAccelerationStructureReadNV;
    m_cmd_bufs[m_frame_index]->pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildNV, vk::PipelineStageFlagBits::eAccelerationStructureBuildNV, {}, 1, &memoryBarrier, 0, 0, 0, 0);

    vk::AccelerationStructureInfoNV buildInfo{};
    buildInfo.type = vk::AccelerationStructureTypeNV::eTopLevel;
    buildInfo.pGeometries = 0;
    buildInfo.geometryCount = 0;
    buildInfo.instanceCount = 1;

    std::vector<GeometryInstance> instances;
    for (auto& mesh : geometry)
    {
        auto res = std::static_pointer_cast<VKResource>(mesh.first);

        instances.emplace_back();
        GeometryInstance& instance = instances.back();
        auto t = mesh.second;
        memcpy(&instance.transform, &t, sizeof(instance.transform));
        instance.instanceId = static_cast<uint32_t>(instances.size() - 1);
        instance.mask = 0xff;
        instance.instanceOffset = 0;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
        instance.accelerationStructureHandle = res->bottom_as.handle;
    }

 
    {
        vk::BufferCreateInfo bufferInfo = {};
        bufferInfo.size = instances.size() * sizeof(instances.back());
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;
        bufferInfo.usage = vk::BufferUsageFlagBits::eRayTracingNV;
        topLevelAS.geometryInstance = m_device->createBufferUnique(bufferInfo);

        vk::MemoryRequirements memRequirements;
        m_device->getBufferMemoryRequirements(topLevelAS.geometryInstance.get(), &memRequirements);

        vk::MemoryAllocateInfo allocInfo = {};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        topLevelAS.geo_memory = m_device->allocateMemoryUnique(allocInfo);

        m_device->bindBufferMemory(topLevelAS.geometryInstance.get(), topLevelAS.geo_memory.get(), 0);

        void* data;
        m_device->mapMemory(topLevelAS.geo_memory.get(), 0, bufferInfo.size, {}, &data);
        memcpy(data, instances.data(), (size_t)bufferInfo.size);
        m_device->unmapMemory(topLevelAS.geo_memory.get());
    }

    m_cmd_bufs[m_frame_index]->buildAccelerationStructureNV(
        &buildInfo,
        topLevelAS.geometryInstance.get(),
        0,
        VK_FALSE,
        topLevelAS.accelerationStructure.get(),
        {},
        topLevelAS.scratchBuffer.get(),
        0
    );

    m_cmd_bufs[m_frame_index]->pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildNV, vk::PipelineStageFlagBits::eAccelerationStructureBuildNV, {}, 1, & memoryBarrier, 0, 0, 0, 0);

    return res;
}

void VKContext::UseProgram(ProgramApi& program)
{
    auto& program_api = static_cast<VKProgramApi&>(program);
    if (m_current_program != &program_api)
    {
        if (m_is_open_render_pass)
        {
            m_cmd_bufs[GetFrameIndex()]->endRenderPass();
            m_is_open_render_pass = false;
        }
    }
    m_current_program = &program_api;
    m_current_program->UseProgram();
}

void VKContext::IASetIndexBuffer(Resource::Ptr ires, gli::format Format)
{
    VKResource::Ptr res = std::static_pointer_cast<VKResource>(ires);
    vk::IndexType index_type = GetVkIndexType(Format);
    m_cmd_bufs[m_frame_index]->bindIndexBuffer(res->buffer.res.get(), 0, index_type);
}

void VKContext::IASetVertexBuffer(uint32_t slot, Resource::Ptr ires)
{
    VKResource::Ptr res = std::static_pointer_cast<VKResource>(ires);
    vk::Buffer vertexBuffers[] = { res->buffer.res.get() };
    vk::DeviceSize offsets[] = { 0 };
    m_cmd_bufs[m_frame_index]->bindVertexBuffers(slot, 1, vertexBuffers, offsets);
}

void VKContext::BeginEvent(const std::string& name)
{
    vk::DebugUtilsLabelEXT label = {};
    label.pLabelName = name.c_str();
    m_cmd_bufs[m_frame_index]->beginDebugUtilsLabelEXT(&label);
}

void VKContext::EndEvent()
{
    m_cmd_bufs[m_frame_index]->endDebugUtilsLabelEXT();
}

void VKContext::DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation)
{
    m_current_program->ApplyBindings();

    auto rp = m_current_program->GetRenderPass();
    auto fb = m_current_program->GetFramebuffer();
    if (rp != m_render_pass || fb != m_framebuffer)
    {
        if (m_is_open_render_pass)
            m_cmd_bufs[GetFrameIndex()]->endRenderPass();
        m_render_pass = rp;
        m_framebuffer = fb;
        m_current_program->RenderPassBegin();
        m_is_open_render_pass = true;
    }
    m_cmd_bufs[m_frame_index]->drawIndexed(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

void VKContext::Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
{
    m_current_program->ApplyBindings();
    m_cmd_bufs[m_frame_index]->dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void VKContext::DispatchRays(uint32_t width, uint32_t height, uint32_t depth)
{
    // Query the ray tracing properties of the current implementation, we will need them later on
    vk::PhysicalDeviceRayTracingPropertiesNV rayTracingProperties{};

    vk::PhysicalDeviceProperties2 deviceProps2{};
    deviceProps2.pNext = &rayTracingProperties;

    m_physical_device.getProperties2(&deviceProps2);

    vk::DeviceSize bindingOffsetRayGenShader = rayTracingProperties.shaderGroupHandleSize * 0;
    vk::DeviceSize bindingOffsetMissShader = rayTracingProperties.shaderGroupHandleSize * 1;
    vk::DeviceSize bindingOffsetHitShader = rayTracingProperties.shaderGroupHandleSize * 2;
    vk::DeviceSize bindingStride = rayTracingProperties.shaderGroupHandleSize;

    m_current_program->ApplyBindings();
    m_cmd_bufs[m_frame_index]->traceRaysNV(
        m_current_program->shaderBindingTable.get(), bindingOffsetRayGenShader,
        m_current_program->shaderBindingTable.get(), bindingOffsetMissShader, bindingStride,
        m_current_program->shaderBindingTable.get(), bindingOffsetHitShader, bindingStride,
        {}, 0, 0,
        width, height, depth
    );
}

Resource::Ptr VKContext::GetBackBuffer()
{
    return m_back_buffers[m_frame_index];
}

void VKContext::CloseCommandBuffer()
{
    if (m_is_open_render_pass)
    {
        m_cmd_bufs[m_frame_index]->endRenderPass();
        m_is_open_render_pass = false;
    }

    TransitionImageLayout(m_back_buffers[m_frame_index]->image, vk::ImageLayout::ePresentSrcKHR, {});

    m_cmd_bufs[m_frame_index]->end();
}

void VKContext::Submit()
{
    vk::SubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_cmd_bufs[m_frame_index].get();
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_image_available_semaphore.get();
    vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eTransfer;
    submitInfo.pWaitDstStageMask = &waitDstStageMask;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_rendering_finished_semaphore.get();

    m_queue.submit(1, &submitInfo, m_fence.get());

    auto res = m_device->waitForFences(1, &m_fence.get(), VK_TRUE, UINT64_MAX);
    if (res != vk::Result::eSuccess)
    {
        throw std::runtime_error("vkWaitForFences");
    }
    m_device->resetFences(1, &m_fence.get());
}

void VKContext::SwapBuffers()
{
    vk::PresentInfoKHR presentInfo = {};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchain.get();
    presentInfo.pImageIndices = &m_frame_index;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_rendering_finished_semaphore.get();

    m_queue.presentKHR(presentInfo);
}

void VKContext::OpenCommandBuffer()
{
    m_device->acquireNextImageKHR(m_swapchain.get(), UINT64_MAX, m_image_available_semaphore.get(), nullptr, &m_frame_index);

    vk::CommandBufferBeginInfo beginInfo = {};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
    m_cmd_bufs[m_frame_index]->begin(beginInfo);
}

void VKContext::Present()
{
    CloseCommandBuffer();
    Submit();
    SwapBuffers();
    OpenCommandBuffer();

    descriptor_pool[m_frame_index]->OnFrameBegin();
    for (auto & x : m_created_program)
        x.get().OnPresent();
}

void VKContext::ResizeBackBuffer(int width, int height)
{
}

VKDescriptorPool& VKContext::GetDescriptorPool()
{
    return *descriptor_pool[m_frame_index];
}

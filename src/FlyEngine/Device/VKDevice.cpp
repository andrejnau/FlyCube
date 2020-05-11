#include "Device/VKDevice.h"
#include <Swapchain/VKSwapchain.h>
#include <CommandList/VKCommandList.h>
#include <Instance/VKInstance.h>
#include <Fence/VKFence.h>
#include <Semaphore/VKSemaphore.h>
#include <Shader/SpirvShader.h>
#include <View/VKView.h>
#include <Adapter/VKAdapter.h>
#include <Utilities/VKUtility.h>
#include <set>

VKDevice::VKDevice(VKAdapter& adapter)
    : m_adapter(adapter)
    , m_physical_device(adapter.GetPhysicalDevice())
{
    auto queue_families = m_physical_device.getQueueFamilyProperties();

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

    auto extensions = m_physical_device.enumerateDeviceExtensionProperties();
    std::set<std::string> req_extension = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
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

    vk::PhysicalDeviceTimelineSemaphoreFeatures device_timetine_feature = {};
    device_timetine_feature.timelineSemaphore = true;

    vk::DeviceCreateInfo device_create_info = {};
    device_create_info.pNext = &device_timetine_feature;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = found_extension.size();
    device_create_info.ppEnabledExtensionNames = found_extension.data();

    m_device = m_physical_device.createDeviceUnique(device_create_info);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device.get());

    m_queue = m_device->getQueue(m_queue_family_index, 0);

    vk::CommandPoolCreateInfo cmd_pool_create_info = {};
    cmd_pool_create_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    cmd_pool_create_info.queueFamilyIndex = m_queue_family_index;
    m_cmd_pool = m_device->createCommandPoolUnique(cmd_pool_create_info);
}

std::shared_ptr<Swapchain> VKDevice::CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count)
{
    return std::make_unique<VKSwapchain>(*this, window, width, height, frame_count);
}

std::shared_ptr<CommandList> VKDevice::CreateCommandList()
{
    return std::make_unique<VKCommandList>(*this);
}

std::shared_ptr<Fence> VKDevice::CreateFence()
{
    return std::make_unique<VKFence>(*this);
}

std::shared_ptr<Semaphore> VKDevice::CreateGPUSemaphore()
{
    return std::make_unique<VKSemaphore>(*this);
}

std::shared_ptr<Resource> VKDevice::CreateTexture(uint32_t bind_flag, gli::format format, uint32_t msaa_count, int width, int height, int depth, int mip_levels)
{
    std::shared_ptr<VKResource> res = std::make_shared<VKResource>(*this);
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
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

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

std::shared_ptr<Resource> VKDevice::CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride)
{
    if (buffer_size == 0)
        return {};

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

    std::shared_ptr<VKResource> res = std::make_shared<VKResource>(*this);
    res->res_type = VKResource::Type::kBuffer;

    res->buffer.res = m_device->createBufferUnique(bufferInfo);

    vk::MemoryRequirements memRequirements;
    m_device->getBufferMemoryRequirements(res->buffer.res.get(), &memRequirements);

    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    res->buffer.memory = m_device->allocateMemoryUnique(allocInfo);

    m_device->bindBufferMemory(res->buffer.res.get(), res->buffer.memory.get(), 0);
    res->buffer.size = buffer_size;

    return res;
}

std::shared_ptr<Resource> VKDevice::CreateSampler(const SamplerDesc& desc)
{
    std::shared_ptr<VKResource> res = std::make_shared<VKResource>(*this);

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

std::shared_ptr<View> VKDevice::CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc)
{
    return std::make_unique<VKView>(*this, resource, view_desc);
}

std::shared_ptr<Shader> VKDevice::CompileShader(const ShaderDesc& desc)
{
    return std::make_unique<SpirvShader>(desc);
}

void VKDevice::Wait(const std::shared_ptr<Semaphore>& semaphore)
{
    std::vector<vk::Semaphore> vk_semaphores;
    if (semaphore)
    {
        VKSemaphore& vk_semaphore = static_cast<VKSemaphore&>(*semaphore);
        vk_semaphores.emplace_back(vk_semaphore.GetSemaphore());
    }
    vk::SubmitInfo submit_info = {};
    submit_info.waitSemaphoreCount = vk_semaphores.size();
    submit_info.pWaitSemaphores = vk_semaphores.data();
    vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eTransfer;
    submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
    m_queue.submit(1, &submit_info, {});
}

void VKDevice::Signal(const std::shared_ptr<Semaphore>& semaphore)
{
    std::vector<vk::Semaphore> vk_semaphores;
    if (semaphore)
    {
        VKSemaphore& vk_semaphore = static_cast<VKSemaphore&>(*semaphore);
        vk_semaphores.emplace_back(vk_semaphore.GetSemaphore());
    }
    vk::SubmitInfo submit_info = {};
    submit_info.signalSemaphoreCount = vk_semaphores.size();
    submit_info.pSignalSemaphores = vk_semaphores.data();
    vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eTransfer;
    submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
    m_queue.submit(1, &submit_info, {});
}

void VKDevice::ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists, const std::shared_ptr<Fence>& fence)
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

    vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eTransfer;
    submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
    m_queue.submit(1, &submit_info, {});

    if (fence)
    {
        VKFence& vk_fence = static_cast<VKFence&>(*fence);
        vk::TimelineSemaphoreSubmitInfo timeline_info = {};
        timeline_info.signalSemaphoreValueCount = 1;
        timeline_info.pSignalSemaphoreValues = &vk_fence.GetValue();

        vk::SubmitInfo signal_submit_info = {};
        signal_submit_info.pNext = &timeline_info;
        signal_submit_info.signalSemaphoreCount = 1;
        signal_submit_info.pSignalSemaphores = &vk_fence.GetFence();
        m_queue.submit(1, &signal_submit_info, {});
    }
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

vk::ImageAspectFlags VKDevice::GetAspectFlags(vk::Format format) const
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

uint32_t VKDevice::FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memProperties;
    m_physical_device.getMemoryProperties(&memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if ((type_filter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

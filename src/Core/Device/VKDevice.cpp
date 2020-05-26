#include "Device/VKDevice.h"
#include <Swapchain/VKSwapchain.h>
#include <CommandList/VKCommandList.h>
#include <Instance/VKInstance.h>
#include <Fence/VKFence.h>
#include <Fence/VKTimelineSemaphore.h>
#include <Semaphore/VKSemaphore.h>
#include <Program/VKProgram.h>
#include <Pipeline/VKGraphicsPipeline.h>
#include <Pipeline/VKComputePipeline.h>
#include <Pipeline/VKRayTracingPipeline.h>
#include <Framebuffer/VKFramebuffer.h>
#include <Shader/SpirvShader.h>
#include <View/VKView.h>
#include <Adapter/VKAdapter.h>
#include <Utilities/VKUtility.h>
#include <set>

struct GeometryInstance
{
    glm::mat3x4 transform;
    uint32_t instanceId : 24;
    uint32_t mask : 8;
    uint32_t instanceOffset : 24;
    uint32_t flags : 8;
    uint64_t accelerationStructureHandle;
};

static vk::IndexType GetVkIndexType(gli::format format)
{
    vk::Format vk_format = static_cast<vk::Format>(format);
    switch (vk_format)
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

VKDevice::VKDevice(VKAdapter& adapter)
    : m_adapter(adapter)
    , m_physical_device(adapter.GetPhysicalDevice())
    , m_gpu_descriptor_pool(*this)
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
        VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME,
        VK_NV_RAY_TRACING_EXTENSION_NAME,
        VK_KHR_MAINTENANCE3_EXTENSION_NAME
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
    };
    if (m_use_timeline_semaphore)
        req_extension.insert(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);

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

uint32_t VKDevice::GetTextureDataPitchAlignment() const
{
    return 1;
}

std::shared_ptr<Swapchain> VKDevice::CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count, bool vsync)
{
    return std::make_shared<VKSwapchain>(*this, window, width, height, frame_count, vsync);
}

std::shared_ptr<CommandList> VKDevice::CreateCommandList()
{
    return std::make_shared<VKCommandList>(*this);
}

std::shared_ptr<Fence> VKDevice::CreateFence()
{
    if (m_use_timeline_semaphore)
        return std::make_shared<VKTimelineSemaphore>(*this);
    else
        return std::make_shared<VKFence>(*this);
}

std::shared_ptr<Semaphore> VKDevice::CreateGPUSemaphore()
{
    return std::make_shared<VKSemaphore>(*this);
}

std::shared_ptr<Resource> VKDevice::CreateTexture(uint32_t bind_flag, gli::format format, uint32_t msaa_count, int width, int height, int depth, int mip_levels)
{
    std::shared_ptr<VKResource> res = std::make_shared<VKResource>(*this);
    res->format = format;
    res->resource_type = ResourceType::kImage;

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
        usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferDst;
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

std::shared_ptr<Resource> VKDevice::CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride, MemoryType memory_type)
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
    else if (bind_flag & BindFlag::kRayTracing)
        bufferInfo.usage = vk::BufferUsageFlagBits::eRayTracingNV;

    if (memory_type == MemoryType::kDefault)
        bufferInfo.usage |= vk::BufferUsageFlagBits::eTransferDst;
    else if (memory_type == MemoryType::kUpload)
        bufferInfo.usage |= vk::BufferUsageFlagBits::eTransferSrc;

    std::shared_ptr<VKResource> res = std::make_shared<VKResource>(*this);
    res->resource_type = ResourceType::kBuffer;
    res->memory_type = memory_type;

    res->buffer.res = m_device->createBufferUnique(bufferInfo);

    vk::MemoryRequirements memRequirements;
    m_device->getBufferMemoryRequirements(res->buffer.res.get(), &memRequirements);

    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.allocationSize = memRequirements.size;

    vk::MemoryPropertyFlags properties = {};
    if (memory_type == MemoryType::kDefault)
        properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
    else if (memory_type == MemoryType::kUpload)
        properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

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

    res->resource_type = ResourceType::kSampler;
    return res;
}

std::shared_ptr<View> VKDevice::CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc)
{
    return std::make_shared<VKView>(*this, std::static_pointer_cast<VKResource>(resource), view_desc);
}

std::shared_ptr<Framebuffer> VKDevice::CreateFramebuffer(const std::shared_ptr<Pipeline>& pipeline, const std::vector<std::shared_ptr<View>>& rtvs, const std::shared_ptr<View>& dsv)
{
    return std::make_shared<VKFramebuffer>(*this, std::static_pointer_cast<VKGraphicsPipeline>(pipeline), rtvs, dsv);
}

std::shared_ptr<Shader> VKDevice::CompileShader(const ShaderDesc& desc)
{
    return std::make_shared<SpirvShader>(desc);
}

std::shared_ptr<Program> VKDevice::CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders)
{
    return std::make_shared<VKProgram>(*this, shaders);
}

std::shared_ptr<Pipeline> VKDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
    return std::make_shared<VKGraphicsPipeline>(*this, desc);
}

std::shared_ptr<Pipeline> VKDevice::CreateComputePipeline(const ComputePipelineDesc& desc)
{
    return std::make_shared<VKComputePipeline>(*this, desc);
}

std::shared_ptr<Pipeline> VKDevice::CreateRayTracingPipeline(const RayTracingPipelineDesc& desc)
{
    return std::make_shared<VKRayTracingPipeline>(*this, desc);
}

std::shared_ptr<Resource> VKDevice::CreateBottomLevelAS(const std::shared_ptr<CommandList>& command_list, const BufferDesc& vertex, const BufferDesc& index)
{
    decltype(auto) vk_command_list = command_list->As<VKCommandList>().GetCommandList();

    std::shared_ptr<VKResource> res = std::make_shared<VKResource>(*this);
    res->resource_type = ResourceType::kBottomLevelAS;
    auto& bottomLevelAS = res->as;

    auto vertex_res = std::static_pointer_cast<VKResource>(vertex.res);
    auto index_res = std::static_pointer_cast<VKResource>(index.res);

    auto vertex_stride = gli::detail::bits_per_pixel(vertex.format) / 8;

    vk::GeometryNV geometry = {};
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
    memoryAllocateInfo.memoryTypeIndex = FindMemoryType(memoryRequirements2.memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    bottomLevelAS.memory = m_device->allocateMemoryUnique(memoryAllocateInfo);

    vk::BindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
    accelerationStructureMemoryInfo.accelerationStructure = bottomLevelAS.accelerationStructure.get();
    accelerationStructureMemoryInfo.memory = bottomLevelAS.memory.get();
    m_device->bindAccelerationStructureMemoryNV(1, &accelerationStructureMemoryInfo);

    m_device->getAccelerationStructureHandleNV(bottomLevelAS.accelerationStructure.get(), sizeof(uint64_t), &bottomLevelAS.handle);

    memoryRequirementsInfo.type = vk::AccelerationStructureMemoryRequirementsTypeNV::eBuildScratch;
    memoryRequirementsInfo.accelerationStructure = bottomLevelAS.accelerationStructure.get();

    vk::MemoryRequirements2 memReqBLAS;
    m_device->getAccelerationStructureMemoryRequirementsNV(&memoryRequirementsInfo, &memReqBLAS);

    auto scratch = std::static_pointer_cast<VKResource>(CreateBuffer(BindFlag::kRayTracing, memReqBLAS.memoryRequirements.size, 0, MemoryType::kDefault));

    vk::AccelerationStructureInfoNV buildInfo{};
    buildInfo.type = vk::AccelerationStructureTypeNV::eBottomLevel;
    buildInfo.instanceCount = 0;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &geometry;

    vk_command_list.buildAccelerationStructureNV(
        buildInfo,
        {},
        0,
        VK_FALSE,
        bottomLevelAS.accelerationStructure.get(),
        {},
        scratch->buffer.res.get(),
        0
    );

    vk::MemoryBarrier memoryBarrier = {};
    memoryBarrier.pNext = nullptr;
    memoryBarrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteNV | vk::AccessFlagBits::eAccelerationStructureReadNV;
    memoryBarrier.dstAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteNV | vk::AccessFlagBits::eAccelerationStructureReadNV;
    vk_command_list.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildNV, vk::PipelineStageFlagBits::eAccelerationStructureBuildNV, {}, 1, &memoryBarrier, 0, 0, 0, 0);

    res->GetPrivateResource(0) = scratch;
    return res;
}

std::shared_ptr<Resource> VKDevice::CreateTopLevelAS(const std::shared_ptr<CommandList>& command_list, const std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4>>& geometry)
{
    decltype(auto) vk_command_list = command_list->As<VKCommandList>().GetCommandList();
    std::shared_ptr<VKResource> res = std::make_shared<VKResource>(*this);
    res->resource_type = ResourceType::kTopLevelAS;
    auto& topLevelAS = res->as;

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
    memoryAllocateInfo.memoryTypeIndex = FindMemoryType(memoryRequirements2.memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    topLevelAS.memory = m_device->allocateMemoryUnique(memoryAllocateInfo);

    vk::BindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
    accelerationStructureMemoryInfo.accelerationStructure = topLevelAS.accelerationStructure.get();
    accelerationStructureMemoryInfo.memory = topLevelAS.memory.get();
    ASSERT_SUCCEEDED(m_device->bindAccelerationStructureMemoryNV(1, &accelerationStructureMemoryInfo));
    ASSERT_SUCCEEDED(m_device->getAccelerationStructureHandleNV(topLevelAS.accelerationStructure.get(), sizeof(uint64_t), &topLevelAS.handle));

    memoryRequirementsInfo.type = vk::AccelerationStructureMemoryRequirementsTypeNV::eBuildScratch;

    vk::AccelerationStructureInfoNV buildInfo{};
    buildInfo.type = vk::AccelerationStructureTypeNV::eTopLevel;
    buildInfo.pGeometries = 0;
    buildInfo.geometryCount = 0;
    buildInfo.instanceCount = 1;

    std::vector<GeometryInstance> instances;
    for (auto& mesh : geometry)
    {
        auto bottom_res = std::static_pointer_cast<VKResource>(mesh.first);

        instances.emplace_back();
        GeometryInstance& instance = instances.back();
        auto t = mesh.second;
        memcpy(&instance.transform, &t, sizeof(instance.transform));
        instance.instanceId = static_cast<uint32_t>(instances.size() - 1);
        instance.mask = 0xff;
        instance.instanceOffset = 0;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
        instance.accelerationStructureHandle = bottom_res->as.handle;
    }


    auto geometry_instance_buffer = std::static_pointer_cast<VKResource>(CreateBuffer(BindFlag::kRayTracing, instances.size() * sizeof(instances.back()), 0, MemoryType::kUpload));


    geometry_instance_buffer->UpdateUploadData(instances.data(), 0, instances.size() * sizeof(instances.back()));

    vk::MemoryRequirements2 memReqTopLevelAS;
    memoryRequirementsInfo.accelerationStructure = topLevelAS.accelerationStructure.get();
    m_device->getAccelerationStructureMemoryRequirementsNV(&memoryRequirementsInfo, &memReqTopLevelAS);

    auto scratch = std::static_pointer_cast<VKResource>(CreateBuffer(BindFlag::kRayTracing, memReqTopLevelAS.memoryRequirements.size, 0, MemoryType::kDefault));

    vk_command_list.buildAccelerationStructureNV(
        &buildInfo,
        geometry_instance_buffer->buffer.res.get(),
        0,
        VK_FALSE,
        topLevelAS.accelerationStructure.get(),
        {},
        scratch->buffer.res.get(),
        0
    );

    vk::MemoryBarrier memoryBarrier = {};
    memoryBarrier.pNext = nullptr;
    memoryBarrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteNV | vk::AccessFlagBits::eAccelerationStructureReadNV;
    memoryBarrier.dstAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteNV | vk::AccessFlagBits::eAccelerationStructureReadNV;
    vk_command_list.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildNV, vk::PipelineStageFlagBits::eAccelerationStructureBuildNV, {}, 1, &memoryBarrier, 0, 0, 0, 0);

    res->GetPrivateResource(0) = scratch;
    res->GetPrivateResource(1) = geometry_instance_buffer;

    return res;
}

bool VKDevice::IsDxrSupported() const
{
    return true;
}

void VKDevice::Wait(const std::shared_ptr<Semaphore>& semaphore)
{
    std::vector<vk::Semaphore> vk_semaphores;
    if (semaphore)
    {
        decltype(auto) vk_semaphore = semaphore->As<VKSemaphore>();
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
        decltype(auto) vk_semaphore = semaphore->As<VKSemaphore>();
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
        decltype(auto) vk_command_list = command_list->As<VKCommandList>();
        vk_command_lists.emplace_back(vk_command_list.GetCommandList());
    }

    vk::SubmitInfo submit_info = {};
    submit_info.commandBufferCount = vk_command_lists.size();
    submit_info.pCommandBuffers = vk_command_lists.data();

    vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eTransfer;
    submit_info.pWaitDstStageMask = &wait_dst_stage_mask;

    vk::Fence handle = {};
    if (!m_use_timeline_semaphore && fence)
    {
        decltype(auto) vk_fence = fence->As<VKFence>();
        handle = vk_fence.GetFence();
    }

    m_queue.submit(1, &submit_info, handle);

    if (m_use_timeline_semaphore && fence)
    {
        decltype(auto) vk_fence = fence->As<VKTimelineSemaphore>();
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

VKGPUDescriptorPool& VKDevice::GetGPUDescriptorPool()
{
    return m_gpu_descriptor_pool;
}

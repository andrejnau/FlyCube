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
    uint32_t instance_offset : 24;
    uint32_t flags : 8;
    uint64_t handle;
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
    device_features.shaderImageGatherExtended = true;

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
    if (format == gli::FORMAT_D24_UNORM_S8_UINT_PACK32)
        format = gli::FORMAT_D32_SFLOAT_S8_UINT_PACK64;

    std::shared_ptr<VKResource> res = std::make_shared<VKResource>(*this);
    res->format = format;
    res->resource_type = ResourceType::kTexture;
    res->image.size.height = height;
    res->image.size.width = width;
    res->image.format = static_cast<vk::Format>(format);
    res->image.level_count = mip_levels;
    res->image.msaa_count = msaa_count;
    res->image.array_layers = depth;

    vk::ImageUsageFlags usage = {};
    if (bind_flag & BindFlag::kDepthStencil)
        usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
    if (bind_flag & BindFlag::kShaderResource)
        usage |= vk::ImageUsageFlagBits::eSampled;
    if (bind_flag & BindFlag::kRenderTarget)
        usage |= vk::ImageUsageFlagBits::eColorAttachment;
    if (bind_flag & BindFlag::kUnorderedAccess)
        usage |= vk::ImageUsageFlagBits::eStorage;
    if (bind_flag & BindFlag::kCopyDest)
        usage |= vk::ImageUsageFlagBits::eTransferDst;
    if (bind_flag & BindFlag::kCopySource)
        usage |= vk::ImageUsageFlagBits::eTransferSrc;

    vk::ImageCreateInfo image_info = {};
    image_info.imageType = vk::ImageType::e2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = mip_levels;
    image_info.arrayLayers = depth;
    image_info.format = res->image.format;
    image_info.tiling = vk::ImageTiling::eOptimal;
    image_info.initialLayout = vk::ImageLayout::eUndefined;
    image_info.usage = usage;
    image_info.samples = static_cast<vk::SampleCountFlagBits>(msaa_count);
    image_info.sharingMode = vk::SharingMode::eExclusive;

    if (image_info.arrayLayers % 6 == 0)
        image_info.flags = vk::ImageCreateFlagBits::eCubeCompatible;

    res->image.res = m_device->createImageUnique(image_info);

    vk::MemoryRequirements mem_requirements;
    m_device->getImageMemoryRequirements(res->image.res.get(), &mem_requirements);

    vk::MemoryAllocateInfo allo_iInfo = {};
    allo_iInfo.allocationSize = mem_requirements.size;
    allo_iInfo.memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

    res->image.memory = m_device->allocateMemoryUnique(allo_iInfo);
    m_device->bindImageMemory(res->image.res.get(), res->image.memory.get(), 0);

    return res;
}

std::shared_ptr<Resource> VKDevice::CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, MemoryType memory_type)
{
    if (buffer_size == 0)
        return {};

    std::shared_ptr<VKResource> res = std::make_shared<VKResource>(*this);
    res->resource_type = ResourceType::kBuffer;
    res->memory_type = memory_type;
    res->buffer.size = buffer_size;

    vk::BufferCreateInfo buffer_info = {};
    buffer_info.size = buffer_size;
    buffer_info.sharingMode = vk::SharingMode::eExclusive;

    if (bind_flag & BindFlag::kVertexBuffer)
        buffer_info.usage |= vk::BufferUsageFlagBits::eVertexBuffer;
    if (bind_flag & BindFlag::kIndexBuffer)
        buffer_info.usage |= vk::BufferUsageFlagBits::eIndexBuffer;
    if (bind_flag & BindFlag::kConstantBuffer)
        buffer_info.usage |= vk::BufferUsageFlagBits::eUniformBuffer;
    if (bind_flag & BindFlag::kShaderResource)
        buffer_info.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
    if (bind_flag & BindFlag::kRayTracing)
        buffer_info.usage |= vk::BufferUsageFlagBits::eRayTracingNV;
    if (bind_flag & BindFlag::kCopySource)
        buffer_info.usage |= vk::BufferUsageFlagBits::eTransferSrc;
    if (bind_flag & BindFlag::kCopyDest)
        buffer_info.usage |= vk::BufferUsageFlagBits::eTransferDst;

    res->buffer.res = m_device->createBufferUnique(buffer_info);

    vk::MemoryRequirements mem_requirements;
    m_device->getBufferMemoryRequirements(res->buffer.res.get(), &mem_requirements);

    vk::MemoryAllocateInfo alloc_info = {};
    alloc_info.allocationSize = mem_requirements.size;

    vk::MemoryPropertyFlags properties = {};
    if (memory_type == MemoryType::kDefault)
        properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
    else if (memory_type == MemoryType::kUpload)
        properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    alloc_info.memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits, properties);
    res->buffer.memory = m_device->allocateMemoryUnique(alloc_info);

    m_device->bindBufferMemory(res->buffer.res.get(), res->buffer.memory.get(), 0);

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

std::shared_ptr<Framebuffer> VKDevice::CreateFramebuffer(const std::shared_ptr<Pipeline>& pipeline, uint32_t width, uint32_t height,
                                                         const std::vector<std::shared_ptr<View>>& rtvs, const std::shared_ptr<View>& dsv)
{
    return std::make_shared<VKFramebuffer>(*this, std::static_pointer_cast<VKGraphicsPipeline>(pipeline), width, height, rtvs, dsv);
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
    auto& bottom_level_as = res->as;

    auto vk_vertex_res = std::static_pointer_cast<VKResource>(vertex.res);
    auto vk_index_res = std::static_pointer_cast<VKResource>(index.res);

    auto vertex_stride = gli::detail::bits_per_pixel(vertex.format) / 8;
    vk::GeometryNV geometry = {};
    geometry.geometryType = vk::GeometryTypeNV::eTriangles;
    geometry.geometry.triangles.vertexData = vk_vertex_res->buffer.res.get();
    geometry.geometry.triangles.vertexOffset = vertex.offset * vertex_stride;
    geometry.geometry.triangles.vertexCount = vertex.count;
    geometry.geometry.triangles.vertexStride = vertex_stride;
    geometry.geometry.triangles.vertexFormat = static_cast<vk::Format>(vertex.format);
    if (vk_index_res)
    {
        auto index_stride = gli::detail::bits_per_pixel(index.format) / 8;
        geometry.geometry.triangles.indexData = vk_index_res->buffer.res.get();
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

    vk::AccelerationStructureInfoNV acceleration_structure_info = {};
    acceleration_structure_info.type = vk::AccelerationStructureTypeNV::eBottomLevel;
    acceleration_structure_info.instanceCount = 0;
    acceleration_structure_info.geometryCount = 1;
    acceleration_structure_info.pGeometries = &geometry;

    vk::AccelerationStructureCreateInfoNV acceleration_structure_ci = {};
    acceleration_structure_ci.info = acceleration_structure_info;
    bottom_level_as.acceleration_structure = m_device->createAccelerationStructureNVUnique(acceleration_structure_ci);

    vk::AccelerationStructureMemoryRequirementsInfoNV memory_requirements_info = {};
    memory_requirements_info.type = vk::AccelerationStructureMemoryRequirementsTypeNV::eObject;
    memory_requirements_info.accelerationStructure = bottom_level_as.acceleration_structure.get();

    vk::MemoryRequirements2 memory_requirements2 = {};
    m_device->getAccelerationStructureMemoryRequirementsNV(&memory_requirements_info, &memory_requirements2);

    vk::MemoryAllocateInfo memory_allocate_info = {};
    memory_allocate_info.allocationSize = memory_requirements2.memoryRequirements.size;
    memory_allocate_info.memoryTypeIndex = FindMemoryType(memory_requirements2.memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    bottom_level_as.memory = m_device->allocateMemoryUnique(memory_allocate_info);

    vk::BindAccelerationStructureMemoryInfoNV acceleration_structure_memory_info = {};
    acceleration_structure_memory_info.accelerationStructure = bottom_level_as.acceleration_structure.get();
    acceleration_structure_memory_info.memory = bottom_level_as.memory.get();
    m_device->bindAccelerationStructureMemoryNV(1, &acceleration_structure_memory_info);
    m_device->getAccelerationStructureHandleNV(bottom_level_as.acceleration_structure.get(), sizeof(uint64_t), &bottom_level_as.handle);

    memory_requirements_info.type = vk::AccelerationStructureMemoryRequirementsTypeNV::eBuildScratch;
    memory_requirements_info.accelerationStructure = bottom_level_as.acceleration_structure.get();

    vk::MemoryRequirements2 mem_req_blas = {};
    m_device->getAccelerationStructureMemoryRequirementsNV(&memory_requirements_info, &mem_req_blas);

    auto scratch = std::static_pointer_cast<VKResource>(CreateBuffer(BindFlag::kRayTracing, mem_req_blas.memoryRequirements.size, MemoryType::kDefault));

    vk::AccelerationStructureInfoNV build_info = {};
    build_info.type = vk::AccelerationStructureTypeNV::eBottomLevel;
    build_info.instanceCount = 0;
    build_info.geometryCount = 1;
    build_info.pGeometries = &geometry;

    vk_command_list.buildAccelerationStructureNV(
        build_info,
        {},
        0,
        VK_FALSE,
        bottom_level_as.acceleration_structure.get(),
        {},
        scratch->buffer.res.get(),
        0
    );

    vk::MemoryBarrier memory_barrier = {};
    memory_barrier.pNext = nullptr;
    memory_barrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteNV | vk::AccessFlagBits::eAccelerationStructureReadNV;
    memory_barrier.dstAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteNV | vk::AccessFlagBits::eAccelerationStructureReadNV;
    vk_command_list.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildNV, vk::PipelineStageFlagBits::eAccelerationStructureBuildNV, {}, 1, &memory_barrier, 0, 0, 0, 0);

    res->GetPrivateResource(0) = scratch;
    return res;
}

std::shared_ptr<Resource> VKDevice::CreateTopLevelAS(const std::shared_ptr<CommandList>& command_list, const std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4>>& geometry)
{
    decltype(auto) vk_command_list = command_list->As<VKCommandList>().GetCommandList();
    std::shared_ptr<VKResource> res = std::make_shared<VKResource>(*this);
    res->resource_type = ResourceType::kTopLevelAS;
    auto& top_level_as = res->as;

    vk::AccelerationStructureInfoNV acceleration_structure_info = {};
    acceleration_structure_info.type = vk::AccelerationStructureTypeNV::eTopLevel;
    acceleration_structure_info.instanceCount = geometry.size();

    vk::AccelerationStructureCreateInfoNV acceleration_structure_ci = {};
    acceleration_structure_ci.info = acceleration_structure_info;
    top_level_as.acceleration_structure = m_device->createAccelerationStructureNVUnique(acceleration_structure_ci);

    vk::AccelerationStructureMemoryRequirementsInfoNV memory_requirements_info = {};
    memory_requirements_info.type = vk::AccelerationStructureMemoryRequirementsTypeNV::eObject;
    memory_requirements_info.accelerationStructure = top_level_as.acceleration_structure.get();

    vk::MemoryRequirements2 memory_requirements2 = {};
    m_device->getAccelerationStructureMemoryRequirementsNV(&memory_requirements_info, &memory_requirements2);

    vk::MemoryAllocateInfo memory_allocate_info = {};
    memory_allocate_info.allocationSize = memory_requirements2.memoryRequirements.size;
    memory_allocate_info.memoryTypeIndex = FindMemoryType(memory_requirements2.memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    top_level_as.memory = m_device->allocateMemoryUnique(memory_allocate_info);

    vk::BindAccelerationStructureMemoryInfoNV acceleration_structure_memory_info = {};
    acceleration_structure_memory_info.accelerationStructure = top_level_as.acceleration_structure.get();
    acceleration_structure_memory_info.memory = top_level_as.memory.get();
    ASSERT_SUCCEEDED(m_device->bindAccelerationStructureMemoryNV(1, &acceleration_structure_memory_info));
    ASSERT_SUCCEEDED(m_device->getAccelerationStructureHandleNV(top_level_as.acceleration_structure.get(), sizeof(uint64_t), &top_level_as.handle));

    vk::AccelerationStructureInfoNV build_info = {};
    build_info.type = vk::AccelerationStructureTypeNV::eTopLevel;
    build_info.instanceCount = geometry.size();

    std::vector<GeometryInstance> instances;
    for (const auto& mesh : geometry)
    {
        auto bottom_res = std::static_pointer_cast<VKResource>(mesh.first);
        GeometryInstance& instance = instances.emplace_back();
        memcpy(&instance.transform, &mesh.second, sizeof(instance.transform));
        instance.instanceId = static_cast<uint32_t>(instances.size() - 1);
        instance.mask = 0xff;
        instance.handle = bottom_res->as.handle;
    }

    auto geometry_instance_buffer = std::static_pointer_cast<VKResource>(CreateBuffer(BindFlag::kRayTracing, instances.size() * sizeof(instances.back()), MemoryType::kUpload));
    geometry_instance_buffer->UpdateUploadData(instances.data(), 0, instances.size() * sizeof(instances.back()));

    vk::MemoryRequirements2 mem_req_top_level_as = {};
    memory_requirements_info.accelerationStructure = top_level_as.acceleration_structure.get();
    memory_requirements_info.type = vk::AccelerationStructureMemoryRequirementsTypeNV::eBuildScratch;
    m_device->getAccelerationStructureMemoryRequirementsNV(&memory_requirements_info, &mem_req_top_level_as);

    auto scratch = std::static_pointer_cast<VKResource>(CreateBuffer(BindFlag::kRayTracing, mem_req_top_level_as.memoryRequirements.size, MemoryType::kDefault));

    vk_command_list.buildAccelerationStructureNV(
        &build_info,
        geometry_instance_buffer->buffer.res.get(),
        0,
        VK_FALSE,
        top_level_as.acceleration_structure.get(),
        {},
        scratch->buffer.res.get(),
        0
    );

    vk::MemoryBarrier memory_barrier = {};
    memory_barrier.pNext = nullptr;
    memory_barrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteNV | vk::AccessFlagBits::eAccelerationStructureReadNV;
    memory_barrier.dstAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteNV | vk::AccessFlagBits::eAccelerationStructureReadNV;
    vk_command_list.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildNV, vk::PipelineStageFlagBits::eAccelerationStructureBuildNV, {}, 1, &memory_barrier, 0, 0, 0, 0);

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

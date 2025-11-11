#include "Device/VKDevice.h"

#include "Adapter/VKAdapter.h"
#include "BindingSet/VKBindingSet.h"
#include "BindingSetLayout/VKBindingSetLayout.h"
#include "CommandList/VKCommandList.h"
#include "CommandQueue/VKCommandQueue.h"
#include "Fence/VKTimelineSemaphore.h"
#include "Instance/VKInstance.h"
#include "Memory/VKMemory.h"
#include "Pipeline/VKComputePipeline.h"
#include "Pipeline/VKGraphicsPipeline.h"
#include "Pipeline/VKRayTracingPipeline.h"
#include "Program/ProgramBase.h"
#include "QueryHeap/VKQueryHeap.h"
#include "Shader/ShaderBase.h"
#include "Swapchain/VKSwapchain.h"
#include "Utilities/Logging.h"
#include "Utilities/NotReached.h"
#include "Utilities/VKUtility.h"
#include "View/VKView.h"

#include <set>
#include <string_view>

namespace {

vk::IndexType GetVkIndexType(gli::format format)
{
    vk::Format vk_format = static_cast<vk::Format>(format);
    switch (vk_format) {
    case vk::Format::eR16Uint:
        return vk::IndexType::eUint16;
    case vk::Format::eR32Uint:
        return vk::IndexType::eUint32;
    default:
        NOTREACHED();
    }
}

std::pair<uint32_t, uint32_t> ConvertShadingRateToPair(ShadingRate shading_rate)
{
    vk::Extent2D size = ConvertShadingRate(shading_rate);
    return { size.width, size.height };
}

} // namespace

vk::ImageLayout ConvertState(ResourceState state)
{
    static std::pair<ResourceState, vk::ImageLayout> mapping[] = {
        { ResourceState::kCommon, vk::ImageLayout::eGeneral },
        { ResourceState::kRenderTarget, vk::ImageLayout::eColorAttachmentOptimal },
        { ResourceState::kUnorderedAccess, vk::ImageLayout::eGeneral },
        { ResourceState::kDepthStencilWrite, vk::ImageLayout::eDepthStencilAttachmentOptimal },
        { ResourceState::kDepthStencilRead, vk::ImageLayout::eDepthStencilReadOnlyOptimal },
        { ResourceState::kNonPixelShaderResource, vk::ImageLayout::eShaderReadOnlyOptimal },
        { ResourceState::kPixelShaderResource, vk::ImageLayout::eShaderReadOnlyOptimal },
        { ResourceState::kCopyDest, vk::ImageLayout::eTransferDstOptimal },
        { ResourceState::kCopySource, vk::ImageLayout::eTransferSrcOptimal },
        { ResourceState::kShadingRateSource, vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR },
        { ResourceState::kPresent, vk::ImageLayout::ePresentSrcKHR },
        { ResourceState::kUndefined, vk::ImageLayout::eUndefined },
    };
    for (const auto& m : mapping) {
        if (state & m.first) {
            assert(state == m.first);
            return m.second;
        }
    }
    NOTREACHED();
}

vk::BuildAccelerationStructureFlagsKHR Convert(BuildAccelerationStructureFlags flags)
{
    vk::BuildAccelerationStructureFlagsKHR vk_flags = {};
    if (flags & BuildAccelerationStructureFlags::kAllowUpdate) {
        vk_flags |= vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
    }
    if (flags & BuildAccelerationStructureFlags::kAllowCompaction) {
        vk_flags |= vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction;
    }
    if (flags & BuildAccelerationStructureFlags::kPreferFastTrace) {
        vk_flags |= vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    }
    if (flags & BuildAccelerationStructureFlags::kPreferFastBuild) {
        vk_flags |= vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild;
    }
    if (flags & BuildAccelerationStructureFlags::kMinimizeMemory) {
        vk_flags |= vk::BuildAccelerationStructureFlagBitsKHR::eLowMemory;
    }
    return vk_flags;
}

vk::Extent2D ConvertShadingRate(ShadingRate shading_rate)
{
    vk::Extent2D fragment_size = { 1, 1 };
    switch (shading_rate) {
    case ShadingRate::k1x1:
        fragment_size.width = 1;
        fragment_size.height = 1;
        break;
    case ShadingRate::k1x2:
        fragment_size.width = 1;
        fragment_size.height = 2;
        break;
    case ShadingRate::k2x1:
        fragment_size.width = 2;
        fragment_size.height = 1;
        break;
    case ShadingRate::k2x2:
        fragment_size.width = 2;
        fragment_size.height = 2;
        break;
    case ShadingRate::k2x4:
        fragment_size.width = 2;
        fragment_size.height = 4;
        break;
    case ShadingRate::k4x2:
        fragment_size.width = 4;
        fragment_size.height = 2;
        break;
    case ShadingRate::k4x4:
        fragment_size.width = 4;
        fragment_size.height = 4;
        break;
    default:
        NOTREACHED();
    }
    return fragment_size;
}

std::array<vk::FragmentShadingRateCombinerOpKHR, 2> ConvertShadingRateCombiners(
    const std::array<ShadingRateCombiner, 2>& combiners)
{
    std::array<vk::FragmentShadingRateCombinerOpKHR, 2> vk_combiners = {};
    for (size_t i = 0; i < vk_combiners.size(); ++i) {
        switch (combiners[i]) {
        case ShadingRateCombiner::kPassthrough:
            vk_combiners[i] = vk::FragmentShadingRateCombinerOpKHR::eKeep;
            break;
        case ShadingRateCombiner::kOverride:
            vk_combiners[i] = vk::FragmentShadingRateCombinerOpKHR::eReplace;
            break;
        case ShadingRateCombiner::kMin:
            vk_combiners[i] = vk::FragmentShadingRateCombinerOpKHR::eMin;
            break;
        case ShadingRateCombiner::kMax:
            vk_combiners[i] = vk::FragmentShadingRateCombinerOpKHR::eMax;
            break;
        case ShadingRateCombiner::kSum:
            vk_combiners[i] = vk::FragmentShadingRateCombinerOpKHR::eMul;
            break;
        default:
            NOTREACHED();
        }
    }
    return vk_combiners;
}

VKDevice::VKDevice(VKAdapter& adapter)
    : m_adapter(adapter)
    , m_physical_device(adapter.GetPhysicalDevice())
    , m_gpu_descriptor_pool(*this)
{
    m_device_properties = m_physical_device.getProperties();
    Logging::Println("{}: Vulkan {}.{}.{}", m_device_properties.deviceName.data(),
                     VK_VERSION_MAJOR(m_device_properties.apiVersion), VK_VERSION_MINOR(m_device_properties.apiVersion),
                     VK_VERSION_PATCH(m_device_properties.apiVersion));

    auto queue_families = m_physical_device.getQueueFamilyProperties();
    auto has_all_bits = [](auto flags, auto bits) { return (flags & bits) == bits; };
    auto has_any_bits = [](auto flags, auto bits) { return flags & bits; };
    for (size_t i = 0; i < queue_families.size(); ++i) {
        const auto& queue = queue_families[i];
        if (queue.queueCount > 0 &&
            has_all_bits(queue.queueFlags,
                         vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer)) {
            m_queues_info[CommandListType::kGraphics].queue_family_index = i;
            m_queues_info[CommandListType::kGraphics].queue_count = queue.queueCount;
        } else if (queue.queueCount > 0 &&
                   has_all_bits(queue.queueFlags, vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer) &&
                   !has_any_bits(queue.queueFlags, vk::QueueFlagBits::eGraphics)) {
            m_queues_info[CommandListType::kCompute].queue_family_index = i;
            m_queues_info[CommandListType::kCompute].queue_count = queue.queueCount;
        } else if (queue.queueCount > 0 && has_all_bits(queue.queueFlags, vk::QueueFlagBits::eTransfer) &&
                   !has_any_bits(queue.queueFlags, vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute)) {
            m_queues_info[CommandListType::kCopy].queue_family_index = i;
            m_queues_info[CommandListType::kCopy].queue_count = queue.queueCount;
        }
    }

    auto extensions = m_physical_device.enumerateDeviceExtensionProperties();
    std::set<std::string_view> requested_extensions = {
        // clang-format off
        VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
        VK_EXT_MESH_SHADER_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        // clang-format on
    };

    if (m_device_properties.apiVersion < VK_API_VERSION_1_3) {
        requested_extensions.insert(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    }

    std::vector<const char*> enabled_extensions;
    std::set<std::string_view> enabled_extension_set;
    for (const auto& extension : extensions) {
        if (requested_extensions.contains(extension.extensionName.data())) {
            enabled_extensions.push_back(extension.extensionName.data());
            enabled_extension_set.insert(extension.extensionName.data());
        }
    }

    const float queue_priority = 1.0;
    std::vector<vk::DeviceQueueCreateInfo> queues_create_info;
    for (const auto& queue_info : m_queues_info) {
        vk::DeviceQueueCreateInfo& queue_create_info = queues_create_info.emplace_back();
        queue_create_info.queueFamilyIndex = queue_info.second.queue_family_index;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
    }

    void* device_create_info_next = nullptr;
    auto add_extension = [&](auto& extension) {
        extension.pNext = device_create_info_next;
        device_create_info_next = &extension;
    };

    vk::PhysicalDeviceFeatures device_features = {};
    vk::PhysicalDeviceFeatures query_device_features = m_physical_device.getFeatures();
    device_features.fragmentStoresAndAtomics = query_device_features.fragmentStoresAndAtomics;
    device_features.geometryShader = query_device_features.geometryShader;
    device_features.imageCubeArray = query_device_features.imageCubeArray;
    device_features.samplerAnisotropy = query_device_features.samplerAnisotropy;
    device_features.sampleRateShading = query_device_features.sampleRateShading;
    device_features.shaderImageGatherExtended = query_device_features.shaderImageGatherExtended;
    device_features.textureCompressionBC = query_device_features.textureCompressionBC;
    device_features.vertexPipelineStoresAndAtomics = query_device_features.vertexPipelineStoresAndAtomics;

    m_geometry_shader_supported = device_features.geometryShader;

    vk::PhysicalDeviceVulkan12Features device_vulkan12_features = {};
    auto query_device_vulkan12_features = GetFeatures2<vk::PhysicalDeviceVulkan12Features>();
    device_vulkan12_features.bufferDeviceAddress = query_device_vulkan12_features.bufferDeviceAddress;
    device_vulkan12_features.drawIndirectCount = query_device_vulkan12_features.drawIndirectCount;
    assert(query_device_vulkan12_features.timelineSemaphore);
    device_vulkan12_features.timelineSemaphore = true;
    device_vulkan12_features.descriptorIndexing = query_device_vulkan12_features.descriptorIndexing;
    device_vulkan12_features.runtimeDescriptorArray = query_device_vulkan12_features.runtimeDescriptorArray;
    device_vulkan12_features.descriptorBindingPartiallyBound =
        query_device_vulkan12_features.descriptorBindingPartiallyBound;
    device_vulkan12_features.descriptorBindingVariableDescriptorCount =
        query_device_vulkan12_features.descriptorBindingVariableDescriptorCount;
    device_vulkan12_features.shaderOutputLayer = query_device_vulkan12_features.shaderOutputLayer;
    device_vulkan12_features.shaderOutputViewportIndex = query_device_vulkan12_features.shaderOutputViewportIndex;

    m_has_buffer_device_address = device_vulkan12_features.bufferDeviceAddress;
    m_draw_indirect_count_supported = device_vulkan12_features.drawIndirectCount;
    m_bindless_supported |= device_vulkan12_features.descriptorIndexing &&
                            device_vulkan12_features.runtimeDescriptorArray &&
                            device_vulkan12_features.descriptorBindingPartiallyBound &&
                            device_vulkan12_features.descriptorBindingVariableDescriptorCount;
    add_extension(device_vulkan12_features);

    vk::PhysicalDeviceVulkan13Features device_vulkan13_features = {};
    vk::PhysicalDeviceDynamicRenderingFeaturesKHR device_dynamic_rendering_features = {};
    if (m_device_properties.apiVersion >= VK_API_VERSION_1_3) {
        assert(GetFeatures2<vk::PhysicalDeviceVulkan13Features>().dynamicRendering);
        device_vulkan13_features.dynamicRendering = true;
        add_extension(device_vulkan13_features);
    } else {
        assert(enabled_extension_set.contains(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME));
        assert(GetFeatures2<vk::PhysicalDeviceDynamicRenderingFeaturesKHR>().dynamicRendering);
        device_dynamic_rendering_features.dynamicRendering = true;
        add_extension(device_dynamic_rendering_features);
    }

    vk::PhysicalDeviceMeshShaderFeaturesEXT mesh_shader_features = {};
    if (enabled_extension_set.contains(VK_EXT_MESH_SHADER_EXTENSION_NAME)) {
        auto query_mesh_shader_features = GetFeatures2<vk::PhysicalDeviceMeshShaderFeaturesEXT>();
        mesh_shader_features.taskShader = query_mesh_shader_features.taskShader;
        mesh_shader_features.meshShader = query_mesh_shader_features.meshShader;
        mesh_shader_features.multiviewMeshShader = query_mesh_shader_features.multiviewMeshShader;
        mesh_shader_features.primitiveFragmentShadingRateMeshShader =
            query_mesh_shader_features.primitiveFragmentShadingRateMeshShader;
        mesh_shader_features.meshShaderQueries = query_mesh_shader_features.meshShaderQueries;

        m_is_mesh_shading_supported = mesh_shader_features.taskShader && mesh_shader_features.meshShader;
        add_extension(mesh_shader_features);
    }

    vk::PhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features = {};
    if (enabled_extension_set.contains(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)) {
        auto query_acceleration_structure_features = GetFeatures2<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();
        acceleration_structure_features.accelerationStructure =
            query_acceleration_structure_features.accelerationStructure;
        add_extension(acceleration_structure_features);
    }

    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR raytracing_pipeline_features = {};
    if (enabled_extension_set.contains(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) &&
        acceleration_structure_features.accelerationStructure) {
        auto ray_tracing_pipeline_properties = GetProperties2<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
        m_shader_group_handle_size = ray_tracing_pipeline_properties.shaderGroupHandleSize;
        m_shader_record_alignment = ray_tracing_pipeline_properties.shaderGroupHandleSize;
        m_shader_table_alignment = ray_tracing_pipeline_properties.shaderGroupBaseAlignment;

        auto query_raytracing_pipeline_features = GetFeatures2<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>();
        raytracing_pipeline_features.rayTracingPipeline = query_raytracing_pipeline_features.rayTracingPipeline;
        raytracing_pipeline_features.rayTraversalPrimitiveCulling =
            query_raytracing_pipeline_features.rayTraversalPrimitiveCulling;

        m_is_dxr_supported = raytracing_pipeline_features.rayTracingPipeline &&
                             raytracing_pipeline_features.rayTraversalPrimitiveCulling;
        add_extension(raytracing_pipeline_features);
    }

    vk::PhysicalDeviceRayQueryFeaturesKHR rayquery_features = {};
    if (enabled_extension_set.contains(VK_KHR_RAY_QUERY_EXTENSION_NAME) &&
        acceleration_structure_features.accelerationStructure) {
        auto query_rayquery_features = GetFeatures2<vk::PhysicalDeviceRayQueryFeaturesKHR>();
        rayquery_features.rayQuery = query_rayquery_features.rayQuery;

        m_is_ray_query_supported = rayquery_features.rayQuery;
        add_extension(rayquery_features);
    }

    vk::PhysicalDeviceFragmentShadingRateFeaturesKHR fragment_shading_rate_features = {};
    if (enabled_extension_set.contains(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME)) {
        auto query_fragment_shading_rate_features = GetFeatures2<vk::PhysicalDeviceFragmentShadingRateFeaturesKHR>();
        fragment_shading_rate_features.pipelineFragmentShadingRate =
            query_fragment_shading_rate_features.pipelineFragmentShadingRate;
        fragment_shading_rate_features.primitiveFragmentShadingRate =
            query_fragment_shading_rate_features.primitiveFragmentShadingRate;
        fragment_shading_rate_features.attachmentFragmentShadingRate =
            query_fragment_shading_rate_features.attachmentFragmentShadingRate;

        auto shading_rate_image_properties = GetProperties2<vk::PhysicalDeviceFragmentShadingRatePropertiesKHR>();
        if (fragment_shading_rate_features.attachmentFragmentShadingRate) {
            std::map<std::pair<uint32_t, uint32_t>, ShadingRate> expected_shading_rates = {
                { ConvertShadingRateToPair(ShadingRate::k1x1), ShadingRate::k1x1 },
                { ConvertShadingRateToPair(ShadingRate::k1x2), ShadingRate::k1x2 },
                { ConvertShadingRateToPair(ShadingRate::k2x1), ShadingRate::k2x1 },
                { ConvertShadingRateToPair(ShadingRate::k2x2), ShadingRate::k2x2 },
                { ConvertShadingRateToPair(ShadingRate::k2x4), ShadingRate::k2x4 },
                { ConvertShadingRateToPair(ShadingRate::k4x2), ShadingRate::k4x2 },
                { ConvertShadingRateToPair(ShadingRate::k4x4), ShadingRate::k4x4 },
            };
            auto fragment_shading_rates = m_physical_device.getFragmentShadingRatesKHR();
            for (const auto& fragment_shading_rate : fragment_shading_rates) {
                vk::Extent2D size = fragment_shading_rate.fragmentSize;
                std::pair<uint32_t, uint32_t> size_as_pair = { size.width, size.height };
                if (!expected_shading_rates.contains(size_as_pair)) {
                    continue;
                }
                uint8_t shading_rate_bits = ((size.width >> 1) << 2) | (size.height >> 1);
                assert((1 << ((shading_rate_bits >> 2) & 3)) == size.width);
                assert((1 << (shading_rate_bits & 3)) == size.height);
                ShadingRate shading_rate = static_cast<ShadingRate>(shading_rate_bits);
                assert(expected_shading_rates.at(size_as_pair) == shading_rate);
                expected_shading_rates.erase(size_as_pair);
            }
            for (const auto& shading_rate : expected_shading_rates) {
                const auto& [width, height] = shading_rate.first;
                Logging::Println("ShadingRate::k{}x{} is not supported", width, height);
            }

            assert(shading_rate_image_properties.minFragmentShadingRateAttachmentTexelSize ==
                   shading_rate_image_properties.maxFragmentShadingRateAttachmentTexelSize);
            assert(shading_rate_image_properties.minFragmentShadingRateAttachmentTexelSize.width ==
                   shading_rate_image_properties.minFragmentShadingRateAttachmentTexelSize.height);
            assert(shading_rate_image_properties.maxFragmentShadingRateAttachmentTexelSize.width ==
                   shading_rate_image_properties.maxFragmentShadingRateAttachmentTexelSize.height);
            assert(shading_rate_image_properties.maxFragmentShadingRateAttachmentTexelSize.width != 0);
        }

        m_is_variable_rate_shading_supported = fragment_shading_rate_features.pipelineFragmentShadingRate;
        m_shading_rate_image_tile_size = shading_rate_image_properties.maxFragmentShadingRateAttachmentTexelSize.width;
        add_extension(fragment_shading_rate_features);
    }

    vk::DeviceCreateInfo device_create_info = {};
    device_create_info.pNext = device_create_info_next;
    device_create_info.queueCreateInfoCount = queues_create_info.size();
    device_create_info.pQueueCreateInfos = queues_create_info.data();
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = enabled_extensions.size();
    device_create_info.ppEnabledExtensionNames = enabled_extensions.data();

    m_device = m_physical_device.createDeviceUnique(device_create_info);

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device.get());
#endif

    for (const auto& queue_info : m_queues_info) {
        vk::CommandPoolCreateInfo cmd_pool_create_info = {};
        cmd_pool_create_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        cmd_pool_create_info.queueFamilyIndex = queue_info.second.queue_family_index;
        m_cmd_pools.emplace(queue_info.first, m_device->createCommandPoolUnique(cmd_pool_create_info));
        m_command_queues[queue_info.first] =
            std::make_shared<VKCommandQueue>(*this, queue_info.first, queue_info.second.queue_family_index);
    }
}

std::shared_ptr<Memory> VKDevice::AllocateMemory(uint64_t size, MemoryType memory_type, uint32_t memory_type_bits)
{
    return std::make_shared<VKMemory>(*this, size, memory_type, memory_type_bits, nullptr);
}

std::shared_ptr<CommandQueue> VKDevice::GetCommandQueue(CommandListType type)
{
    return m_command_queues.at(GetAvailableCommandListType(type));
}

uint32_t VKDevice::GetTextureDataPitchAlignment() const
{
    return 1;
}

std::shared_ptr<Swapchain> VKDevice::CreateSwapchain(WindowHandle window,
                                                     uint32_t width,
                                                     uint32_t height,
                                                     uint32_t frame_count,
                                                     bool vsync)
{
    return std::make_shared<VKSwapchain>(*m_command_queues.at(CommandListType::kGraphics), window, width, height,
                                         frame_count, vsync);
}

std::shared_ptr<CommandList> VKDevice::CreateCommandList(CommandListType type)
{
    return std::make_shared<VKCommandList>(*this, type);
}

std::shared_ptr<Fence> VKDevice::CreateFence(uint64_t initial_value)
{
    return std::make_shared<VKTimelineSemaphore>(*this, initial_value);
}

MemoryRequirements VKDevice::GetTextureMemoryRequirements(const TextureDesc& desc)
{
    return VKResource::CreateImage(*this, desc)->GetMemoryRequirements();
}

MemoryRequirements VKDevice::GetMemoryBufferRequirements(const BufferDesc& desc)
{
    return VKResource::CreateBuffer(*this, desc)->GetMemoryRequirements();
}

std::shared_ptr<Resource> VKDevice::CreatePlacedTexture(const std::shared_ptr<Memory>& memory,
                                                        uint64_t offset,
                                                        const TextureDesc& desc)
{
    auto texture = VKResource::CreateImage(*this, desc);
    if (texture) {
        texture->BindMemory(memory, offset);
    }
    return texture;
}

std::shared_ptr<Resource> VKDevice::CreatePlacedBuffer(const std::shared_ptr<Memory>& memory,
                                                       uint64_t offset,
                                                       const BufferDesc& desc)
{
    auto buffer = VKResource::CreateBuffer(*this, desc);
    if (buffer) {
        buffer->BindMemory(memory, offset);
    }
    return buffer;
}

std::shared_ptr<Resource> VKDevice::CreateTexture(MemoryType memory_type, const TextureDesc& desc)
{
    auto texture = VKResource::CreateImage(*this, desc);
    if (texture) {
        texture->CommitMemory(memory_type);
    }
    return texture;
}

std::shared_ptr<Resource> VKDevice::CreateBuffer(MemoryType memory_type, const BufferDesc& desc)
{
    auto buffer = VKResource::CreateBuffer(*this, desc);
    if (buffer) {
        buffer->CommitMemory(memory_type);
    }
    return buffer;
}

std::shared_ptr<Resource> VKDevice::CreateSampler(const SamplerDesc& desc)
{
    return VKResource::CreateSampler(*this, desc);
}

std::shared_ptr<View> VKDevice::CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc)
{
    return std::make_shared<VKView>(*this, std::static_pointer_cast<VKResource>(resource), view_desc);
}

std::shared_ptr<BindingSetLayout> VKDevice::CreateBindingSetLayout(const std::vector<BindKey>& descs)
{
    return std::make_shared<VKBindingSetLayout>(*this, descs);
}

std::shared_ptr<BindingSet> VKDevice::CreateBindingSet(const std::shared_ptr<BindingSetLayout>& layout)
{
    return std::make_shared<VKBindingSet>(*this, std::static_pointer_cast<VKBindingSetLayout>(layout));
}

std::shared_ptr<Shader> VKDevice::CreateShader(const std::vector<uint8_t>& blob,
                                               ShaderBlobType blob_type,
                                               ShaderType shader_type)
{
    return std::make_shared<ShaderBase>(blob, blob_type, shader_type);
}

std::shared_ptr<Shader> VKDevice::CompileShader(const ShaderDesc& desc)
{
    return std::make_shared<ShaderBase>(desc, ShaderBlobType::kSPIRV);
}

std::shared_ptr<Program> VKDevice::CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders)
{
    return std::make_shared<ProgramBase>(shaders);
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

vk::AccelerationStructureGeometryKHR VKDevice::FillRaytracingGeometryTriangles(
    const RaytracingGeometryBufferDesc& vertex,
    const RaytracingGeometryBufferDesc& index,
    RaytracingGeometryFlags flags) const
{
    vk::AccelerationStructureGeometryKHR geometry_desc = {};
    geometry_desc.geometryType = vk::GeometryTypeKHR::eTriangles;
    switch (flags) {
    case RaytracingGeometryFlags::kOpaque:
        geometry_desc.flags = vk::GeometryFlagBitsKHR::eOpaque;
        break;
    case RaytracingGeometryFlags::kNoDuplicateAnyHitInvocation:
        geometry_desc.flags = vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation;
        break;
    default:
        break;
    }

    auto vk_vertex_res = std::static_pointer_cast<VKResource>(vertex.res);
    auto vk_index_res = std::static_pointer_cast<VKResource>(index.res);

    auto vertex_stride = gli::detail::bits_per_pixel(vertex.format) / 8;
    geometry_desc.geometry.triangles.vertexData =
        m_device->getBufferAddress({ vk_vertex_res->GetBuffer() }) + vertex.offset * vertex_stride;
    geometry_desc.geometry.triangles.vertexStride = vertex_stride;
    geometry_desc.geometry.triangles.vertexFormat = static_cast<vk::Format>(vertex.format);
    geometry_desc.geometry.triangles.maxVertex = vertex.count;
    if (vk_index_res) {
        auto index_stride = gli::detail::bits_per_pixel(index.format) / 8;
        geometry_desc.geometry.triangles.indexData =
            m_device->getBufferAddress({ vk_index_res->GetBuffer() }) + index.offset * index_stride;
        geometry_desc.geometry.triangles.indexType = GetVkIndexType(index.format);
    } else {
        geometry_desc.geometry.triangles.indexType = vk::IndexType::eNoneKHR;
    }

    return geometry_desc;
}

RaytracingASPrebuildInfo VKDevice::GetAccelerationStructurePrebuildInfo(
    const vk::AccelerationStructureBuildGeometryInfoKHR& acceleration_structure_info,
    const std::vector<uint32_t>& max_primitive_counts) const
{
    vk::AccelerationStructureBuildSizesInfoKHR size_info = {};
    m_device->getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice,
                                                    &acceleration_structure_info, max_primitive_counts.data(),
                                                    &size_info);
    RaytracingASPrebuildInfo prebuild_info = {};
    prebuild_info.acceleration_structure_size = size_info.accelerationStructureSize;
    prebuild_info.build_scratch_data_size = size_info.buildScratchSize;
    prebuild_info.update_scratch_data_size = size_info.updateScratchSize;
    return prebuild_info;
}

std::shared_ptr<Resource> VKDevice::CreateAccelerationStructure(const AccelerationStructureDesc& desc)
{
    return VKResource::CreateAccelerationStructure(*this, desc);
}

std::shared_ptr<QueryHeap> VKDevice::CreateQueryHeap(QueryHeapType type, uint32_t count)
{
    return std::make_shared<VKQueryHeap>(*this, type, count);
}

bool VKDevice::IsDxrSupported() const
{
    return m_is_dxr_supported;
}

bool VKDevice::IsRayQuerySupported() const
{
    return m_is_ray_query_supported;
}

bool VKDevice::IsVariableRateShadingSupported() const
{
    return m_is_variable_rate_shading_supported;
}

bool VKDevice::IsMeshShadingSupported() const
{
    return m_is_mesh_shading_supported;
}

bool VKDevice::IsDrawIndirectCountSupported() const
{
    return m_draw_indirect_count_supported;
}

bool VKDevice::IsGeometryShaderSupported() const
{
    return m_geometry_shader_supported;
}

bool VKDevice::IsBindlessSupported() const
{
    return m_bindless_supported;
}

uint32_t VKDevice::GetShadingRateImageTileSize() const
{
    return m_shading_rate_image_tile_size;
}

MemoryBudget VKDevice::GetMemoryBudget() const
{
    auto memory_budget = GetMemoryProperties2<vk::PhysicalDeviceMemoryBudgetPropertiesEXT>();
    MemoryBudget res = {};
    for (size_t i = 0; i < VK_MAX_MEMORY_HEAPS; ++i) {
        res.budget += memory_budget.heapBudget[i];
        res.usage += memory_budget.heapUsage[i];
    }
    return res;
}

uint32_t VKDevice::GetShaderGroupHandleSize() const
{
    return m_shader_group_handle_size;
}

uint32_t VKDevice::GetShaderRecordAlignment() const
{
    return m_shader_record_alignment;
}

uint32_t VKDevice::GetShaderTableAlignment() const
{
    return m_shader_table_alignment;
}

RaytracingASPrebuildInfo VKDevice::GetBLASPrebuildInfo(const std::vector<RaytracingGeometryDesc>& descs,
                                                       BuildAccelerationStructureFlags flags) const
{
    std::vector<vk::AccelerationStructureGeometryKHR> geometry_descs;
    std::vector<uint32_t> max_primitive_counts;
    for (const auto& desc : descs) {
        geometry_descs.emplace_back(FillRaytracingGeometryTriangles(desc.vertex, desc.index, desc.flags));
        if (desc.index.res) {
            max_primitive_counts.emplace_back(desc.index.count / 3);
        } else {
            max_primitive_counts.emplace_back(desc.vertex.count / 3);
        }
    }
    vk::AccelerationStructureBuildGeometryInfoKHR acceleration_structure_info = {};
    acceleration_structure_info.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
    acceleration_structure_info.geometryCount = geometry_descs.size();
    acceleration_structure_info.pGeometries = geometry_descs.data();
    acceleration_structure_info.flags = Convert(flags);
    return GetAccelerationStructurePrebuildInfo(acceleration_structure_info, max_primitive_counts);
}

RaytracingASPrebuildInfo VKDevice::GetTLASPrebuildInfo(uint32_t instance_count,
                                                       BuildAccelerationStructureFlags flags) const
{
    vk::AccelerationStructureGeometryKHR geometry_info{};
    geometry_info.geometryType = vk::GeometryTypeKHR::eInstances;
    geometry_info.geometry.setInstances({});

    vk::AccelerationStructureBuildGeometryInfoKHR acceleration_structure_info = {};
    acceleration_structure_info.type = vk::AccelerationStructureTypeKHR::eTopLevel;
    acceleration_structure_info.pGeometries = &geometry_info;
    acceleration_structure_info.geometryCount = 1;
    acceleration_structure_info.flags = Convert(flags);
    return GetAccelerationStructurePrebuildInfo(acceleration_structure_info, { instance_count });
}

ShaderBlobType VKDevice::GetSupportedShaderBlobType() const
{
    return ShaderBlobType::kSPIRV;
}

VKAdapter& VKDevice::GetAdapter()
{
    return m_adapter;
}

vk::Device VKDevice::GetDevice()
{
    return m_device.get();
}

CommandListType VKDevice::GetAvailableCommandListType(CommandListType type)
{
    if (m_queues_info.contains(type)) {
        return type;
    }
    return CommandListType::kGraphics;
}

vk::CommandPool VKDevice::GetCmdPool(CommandListType type)
{
    return m_cmd_pools.at(GetAvailableCommandListType(type)).get();
}

vk::ImageAspectFlags VKDevice::GetAspectFlags(vk::Format format) const
{
    switch (format) {
    case vk::Format::eD32SfloatS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD16UnormS8Uint:
        return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    case vk::Format::eD16Unorm:
    case vk::Format::eD32Sfloat:
    case vk::Format::eX8D24UnormPack32:
        return vk::ImageAspectFlagBits::eDepth;
    case vk::Format::eS8Uint:
        return vk::ImageAspectFlagBits::eStencil;
    default:
        return vk::ImageAspectFlagBits::eColor;
    }
}

uint32_t VKDevice::FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties mem_properties = m_physical_device.getMemoryProperties();
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i) {
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    NOTREACHED();
}

VKGPUBindlessDescriptorPoolTyped& VKDevice::GetGPUBindlessDescriptorPool(vk::DescriptorType type)
{
    auto it = m_gpu_bindless_descriptor_pool.find(type);
    if (it == m_gpu_bindless_descriptor_pool.end()) {
        it = m_gpu_bindless_descriptor_pool
                 .emplace(std::piecewise_construct, std::forward_as_tuple(type), std::forward_as_tuple(*this, type))
                 .first;
    }
    return it->second;
}

VKGPUDescriptorPool& VKDevice::GetGPUDescriptorPool()
{
    return m_gpu_descriptor_pool;
}

uint32_t VKDevice::GetMaxDescriptorSetBindings(vk::DescriptorType type) const
{
    switch (type) {
    case vk::DescriptorType::eSampler:
        return m_device_properties.limits.maxPerStageDescriptorSamplers;
    case vk::DescriptorType::eCombinedImageSampler:
    case vk::DescriptorType::eSampledImage:
    case vk::DescriptorType::eUniformTexelBuffer:
        return m_device_properties.limits.maxPerStageDescriptorSampledImages;
    case vk::DescriptorType::eUniformBuffer:
    case vk::DescriptorType::eUniformBufferDynamic:
        return m_device_properties.limits.maxPerStageDescriptorUniformBuffers;
    case vk::DescriptorType::eStorageBuffer:
    case vk::DescriptorType::eStorageBufferDynamic:
        return m_device_properties.limits.maxPerStageDescriptorStorageBuffers;
    case vk::DescriptorType::eStorageImage:
    case vk::DescriptorType::eStorageTexelBuffer:
        return m_device_properties.limits.maxPerStageDescriptorStorageImages;
    default:
        NOTREACHED();
    }
}

bool VKDevice::HasBufferDeviceAddress() const
{
    return m_has_buffer_device_address;
}

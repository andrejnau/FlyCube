#include "Instance/VKInstance.h"
#include <Adapter/VKAdapter.h>
#include <set>
#include <string>
#include <sstream>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

class DebugReportListener
{
public:
    DebugReportListener(const vk::Instance& instance)
    {
        if (!enabled)
            return;
        vk::DebugReportCallbackCreateInfoEXT callback_create_info = {};
        callback_create_info.flags = vk::DebugReportFlagBitsEXT::eWarning |
                                     vk::DebugReportFlagBitsEXT::ePerformanceWarning |
                                     vk::DebugReportFlagBitsEXT::eError |
                                     vk::DebugReportFlagBitsEXT::eDebug;
        callback_create_info.pfnCallback = &DebugReportCallback;
        callback_create_info.pUserData = this;
        m_callback = instance.createDebugReportCallbackEXTUnique(callback_create_info);
    }

#if defined(_DEBUG)
    static constexpr bool enabled = true;
#else
    static constexpr bool enabled = false;
#endif

private:
    static bool SkipIt(VkDebugReportObjectTypeEXT object_type, const std::string& message)
    {
        static std::vector<std::string> muted_warnings = {
            "UNASSIGNED-CoreValidation-Shader-InconsistentSpirv",
            "UNASSIGNED-CoreValidation-Shader-OutputNotConsumed",
            "UNASSIGNED-CoreValidation-Shader-InterfaceTypeMismatch",
            "UNASSIGNED-CoreValidation-DrawState-InvalidImageLayout",
            "UNASSIGNED-CoreValidation-DrawState-DescriptorSetNotUpdated",
            "UNASSIGNED-CoreValidation-DrawState-InvalidImageLayout",
            "VUID-vkCmdDrawIndexed-None-02720",
            "VUID-vkCmdPipelineBarrier-oldLayout-01181",
            "VUID-vkCmdClearDepthStencilImage-imageLayout",
            "VUID-vkCmdClearDepthStencilImage-renderpass",
            "VUID-vkCmdClearColorImage-imageLayout",
            "VUID-VkFramebufferCreateInfo-pAttachments",
            "VUID-VkRenderPassCreateInfo-pDependencies",
            "VUID-VkDescriptorImageInfo-imageLayout"
        };
        for (auto& str : muted_warnings)
        {
            if (message.find(str) != std::string::npos)
                return true;
        }
        return false;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
        VkDebugReportFlagsEXT       flags,
        VkDebugReportObjectTypeEXT  objectType,
        uint64_t                    object,
        size_t                      location,
        int32_t                     messageCode,
        const char* pLayerPrefix,
        const char* pMessage,
        void* pUserData)
    {
        constexpr size_t error_limit = 1024;
        DebugReportListener& self = *reinterpret_cast<DebugReportListener*>(pUserData);
        if (self.m_error_count >= error_limit || SkipIt(objectType, pMessage))
            return VK_FALSE;
#ifdef _WIN32
        if (self.m_error_count < error_limit)
        {
            std::stringstream buf;
            buf << pLayerPrefix << " " << to_string(static_cast<vk::DebugReportFlagBitsEXT>(flags)) << " " << pMessage << std::endl;
            OutputDebugStringA(buf.str().c_str());
        }
#endif
        ++self.m_error_count;
        return VK_FALSE;
    }

    vk::UniqueDebugReportCallbackEXT m_callback;
    size_t m_error_count = 0;
};

VKInstance::VKInstance()
{
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = m_dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

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
    app_info.apiVersion = VK_API_VERSION_1_2;

    vk::InstanceCreateInfo create_info;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledLayerCount = static_cast<uint32_t>(found_layers.size());
    create_info.ppEnabledLayerNames = found_layers.data();
    create_info.enabledExtensionCount = static_cast<uint32_t>(found_extension.size());
    create_info.ppEnabledExtensionNames = found_extension.data();

    m_instance = vk::createInstanceUnique(create_info);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance.get());
    static DebugReportListener listener(m_instance.get());
}

std::vector<std::shared_ptr<Adapter>> VKInstance::EnumerateAdapters()
{
    std::vector<std::shared_ptr<Adapter>> adapters;
    auto devices = m_instance->enumeratePhysicalDevices();
    for (const auto& device : devices)
    {
        vk::PhysicalDeviceProperties device_properties = device.getProperties();

        if (device_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ||
            device_properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
        {
            adapters.emplace_back(std::make_shared<VKAdapter>(*this, device));
        }
    }
    return adapters;
}

vk::Instance& VKInstance::GetInstance()
{
    return m_instance.get();
}

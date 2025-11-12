#include "Instance/VKInstance.h"

#include "Adapter/VKAdapter.h"
#include "Utilities/Check.h"
#include "Utilities/Logging.h"

#include <set>
#include <string>
#include <string_view>

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

namespace {

bool SkipMessage(std::string_view message)
{
    static constexpr std::string_view kMutedMessages[] = {
        "SPV_GOOGLE_hlsl_functionality1",
        "SPV_GOOGLE_user_type",
        "VUID-VkDeviceCreateInfo-pProperties-04451",
    };
    for (const auto& str : kMutedMessages) {
        if (message.find(str) != std::string::npos) {
            return true;
        }
    }
    return false;
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL
DebugUtilsMessengerCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                            vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
                            const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                            void* pUserData)
{
    std::string_view message(pCallbackData->pMessage);
    std::string message_prefix(message.substr(0, message.find(']')));
    static std::set<std::string> message_prefixes;
    if (SkipMessage(message) || message_prefixes.contains(message_prefix)) {
        return vk::False;
    }
    message_prefixes.emplace(message_prefix);

    static constexpr size_t kErrorLimit = 1024;
    static size_t error_count = 0;
    if (++error_count > kErrorLimit) {
        return vk::False;
    }

    Logging::Println("[VK_EXT_debug_utils] {}", message);
    return vk::False;
}

bool IsValidationEnabled()
{
#if defined(_WIN32)
    return IsDebuggerPresent();
#else
    return true;
#endif
}

} // namespace

VKInstance::VKInstance()
{
#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
        m_dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
#endif

    std::set<std::string_view> requested_layers;
    if (IsValidationEnabled()) {
        requested_layers.insert("VK_LAYER_KHRONOS_validation");
    }

    auto layers = vk::enumerateInstanceLayerProperties();
    std::vector<const char*> enabled_layers;
    for (const auto& layer : layers) {
        if (requested_layers.contains(layer.layerName.data())) {
            enabled_layers.push_back(layer.layerName);
        }
    }

    std::set<std::string_view> requested_extensions = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XCB_KHR)
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_METAL_EXT)
        VK_EXT_METAL_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#endif
    };

    auto extensions = vk::enumerateInstanceExtensionProperties();
    std::vector<const char*> enabled_extensions;
    std::set<std::string_view> enabled_extension_set;
    for (const auto& extension : extensions) {
        if (requested_extensions.contains(extension.extensionName.data())) {
            enabled_extensions.push_back(extension.extensionName.data());
            enabled_extension_set.insert(extension.extensionName.data());
        }
    }

    vk::ApplicationInfo app_info = {};
    app_info.apiVersion = vk::enumerateInstanceVersion();
    CHECK(app_info.apiVersion >= VK_API_VERSION_1_1);

    vk::InstanceCreateInfo instance_info;
    if (enabled_extension_set.contains(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
        instance_info.flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
    }
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledLayerCount = enabled_layers.size();
    instance_info.ppEnabledLayerNames = enabled_layers.data();
    instance_info.enabledExtensionCount = enabled_extensions.size();
    instance_info.ppEnabledExtensionNames = enabled_extensions.data();
    m_instance = vk::createInstanceUnique(instance_info);

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance.get());
#endif

    m_debug_utils_supported = enabled_extension_set.contains(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    if (IsValidationEnabled() && m_debug_utils_supported) {
        vk::DebugUtilsMessengerCreateInfoEXT debug_utils_messenger_info = {};
        debug_utils_messenger_info.messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
        debug_utils_messenger_info.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                 vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                                 vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                 vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding;
        debug_utils_messenger_info.pfnUserCallback = &DebugUtilsMessengerCallback;
        debug_utils_messenger_info.pUserData = this;
        m_debug_utils_messenger = m_instance->createDebugUtilsMessengerEXTUnique(debug_utils_messenger_info);
    }
}

std::vector<std::shared_ptr<Adapter>> VKInstance::EnumerateAdapters()
{
    std::vector<std::shared_ptr<Adapter>> adapters;
    std::vector<std::shared_ptr<Adapter>> sortware_adapters;
    auto physical_devices = m_instance->enumeratePhysicalDevices();
    for (const auto& physical_device : physical_devices) {
        vk::PhysicalDeviceProperties properties = physical_device.getProperties();
        if (properties.apiVersion < VK_API_VERSION_1_2) {
            continue;
        }
        if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ||
            properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
            adapters.emplace_back(std::make_shared<VKAdapter>(*this, physical_device));
        } else if (properties.deviceType == vk::PhysicalDeviceType::eCpu) {
            sortware_adapters.emplace_back(std::make_shared<VKAdapter>(*this, physical_device));
        }
    }
    if (adapters.empty()) {
        return sortware_adapters;
    }
    return adapters;
}

vk::Instance& VKInstance::GetInstance()
{
    return m_instance.get();
}

bool VKInstance::IsDebugUtilsSupported() const
{
    return m_debug_utils_supported;
}

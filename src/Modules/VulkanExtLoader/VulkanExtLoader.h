#include <vulkan/vulkan.h>

namespace ext
{
    extern PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
    extern PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructureNV;
    extern PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructureNV;
    extern PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemoryNV;
    extern PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandleNV;
    extern PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV;
    extern PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructureNV;
    extern PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV;
    extern PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV;
    extern PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV;
}

void LoadVkInstanceExt(VkInstance instance);
void LoadVkDeviceExt(VkDevice device);

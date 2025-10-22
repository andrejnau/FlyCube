#include "Utilities/NotReached.h"

#include <vulkan/vulkan.hpp>

__attribute__((weak)) VkResult vkCreateAccelerationStructureKHR(VkDevice device,
                                                                const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
                                                                const VkAllocationCallbacks* pAllocator,
                                                                VkAccelerationStructureKHR* pAccelerationStructure)
{
    NOTREACHED();
}

__attribute__((weak)) void vkDestroyAccelerationStructureKHR(VkDevice device,
                                                             VkAccelerationStructureKHR accelerationStructure,
                                                             const VkAllocationCallbacks* pAllocator)
{
    NOTREACHED();
}

__attribute__((weak)) VkDeviceAddress
vkGetAccelerationStructureDeviceAddressKHR(VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR* pInfo)
{
    NOTREACHED();
}

__attribute__((weak)) void vkGetAccelerationStructureBuildSizesKHR(
    VkDevice device,
    VkAccelerationStructureBuildTypeKHR buildType,
    const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
    const uint32_t* pMaxPrimitiveCounts,
    VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo)
{
    NOTREACHED();
}

__attribute__((weak)) VkResult vkGetRayTracingShaderGroupHandlesKHR(VkDevice device,
                                                                    VkPipeline pipeline,
                                                                    uint32_t firstGroup,
                                                                    uint32_t groupCount,
                                                                    size_t dataSize,
                                                                    void* pData)
{
    NOTREACHED();
}

__attribute__((weak)) VkResult vkCreateRayTracingPipelinesKHR(VkDevice device,
                                                              VkDeferredOperationKHR deferredOperation,
                                                              VkPipelineCache pipelineCache,
                                                              uint32_t createInfoCount,
                                                              const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
                                                              const VkAllocationCallbacks* pAllocator,
                                                              VkPipeline* pPipelines)
{
    NOTREACHED();
}

__attribute__((weak)) void vkCmdBuildAccelerationStructuresKHR(
    VkCommandBuffer commandBuffer,
    uint32_t infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
    const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos)
{
    NOTREACHED();
}

__attribute__((weak)) void vkCmdCopyAccelerationStructureKHR(VkCommandBuffer commandBuffer,
                                                             const VkCopyAccelerationStructureInfoKHR* pInfo)
{
    NOTREACHED();
}

__attribute__((weak)) void vkCmdWriteAccelerationStructuresPropertiesKHR(
    VkCommandBuffer commandBuffer,
    uint32_t accelerationStructureCount,
    const VkAccelerationStructureKHR* pAccelerationStructures,
    VkQueryType queryType,
    VkQueryPool queryPool,
    uint32_t firstQuery)
{
    NOTREACHED();
}

__attribute__((weak)) void vkCmdTraceRaysKHR(VkCommandBuffer commandBuffer,
                                             const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable,
                                             const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable,
                                             const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable,
                                             const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable,
                                             uint32_t width,
                                             uint32_t height,
                                             uint32_t depth)
{
    NOTREACHED();
}

__attribute__((weak)) void vkCmdDrawMeshTasksEXT(VkCommandBuffer commandBuffer,
                                                 uint32_t groupCountX,
                                                 uint32_t groupCountY,
                                                 uint32_t groupCountZ)
{
    NOTREACHED();
}

__attribute__((weak)) VkResult
vkGetPhysicalDeviceFragmentShadingRatesKHR(VkPhysicalDevice physicalDevice,
                                           uint32_t* pFragmentShadingRateCount,
                                           VkPhysicalDeviceFragmentShadingRateKHR* pFragmentShadingRates)
{
    NOTREACHED();
}

__attribute__((weak)) void vkCmdSetFragmentShadingRateKHR(VkCommandBuffer commandBuffer,
                                                          const VkExtent2D* pFragmentSize,
                                                          const VkFragmentShadingRateCombinerOpKHR combinerOps[2])
{
    NOTREACHED();
}

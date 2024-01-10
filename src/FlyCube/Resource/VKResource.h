#pragma once
#include "Resource/ResourceBase.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include <map>

inline bool operator<(const VkImageSubresourceRange& lhs, const VkImageSubresourceRange& rhs)
{
    return std::tie(lhs.aspectMask, lhs.baseArrayLayer, lhs.baseMipLevel, lhs.layerCount, lhs.levelCount) <
           std::tie(rhs.aspectMask, rhs.baseArrayLayer, rhs.baseMipLevel, rhs.layerCount, rhs.levelCount);
}

class VKDevice;

class VKResource : public ResourceBase {
public:
    VKResource(VKDevice& device);

    void CommitMemory(MemoryType memory_type) override;
    void BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset) override;
    uint64_t GetWidth() const override;
    uint32_t GetHeight() const override;
    uint16_t GetLayerCount() const override;
    uint16_t GetLevelCount() const override;
    uint32_t GetSampleCount() const override;
    uint64_t GetAccelerationStructureHandle() const override;
    void SetName(const std::string& name) override;
    uint8_t* Map() override;
    void Unmap() override;
    bool AllowCommonStatePromotion(ResourceState state_after) override;
    MemoryRequirements GetMemoryRequirements() const override;

    struct Image {
        vk::Image res;
        vk::UniqueImage res_owner;
        vk::Format format = vk::Format::eUndefined;
        vk::Extent2D size = {};
        uint32_t level_count = 1;
        uint32_t sample_count = 1;
        uint32_t array_layers = 1;
    } image;

    struct Buffer {
        vk::UniqueBuffer res;
        uint32_t size = 0;
    } buffer;

    struct Sampler {
        vk::UniqueSampler res;
    } sampler;

#ifndef USE_STATIC_MOLTENVK
    vk::UniqueAccelerationStructureKHR acceleration_structure_handle = {};
#else
    struct {
        vk::AccelerationStructureKHR value = {};
        vk::AccelerationStructureKHR& get()
        {
            return value;
        }
    } acceleration_structure_handle = {};
#endif

private:
    VKDevice& m_device;
    vk::DeviceMemory m_vk_memory;
};

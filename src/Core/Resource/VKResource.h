#pragma once
#include "Resource/ResourceBase.h"
#include <glm/glm.hpp>
#include <map>
#include <Utilities/Vulkan.h>

static bool operator<(const VkImageSubresourceRange& lhs, const VkImageSubresourceRange& rhs)
{
    return std::tie(lhs.aspectMask, lhs.baseArrayLayer, lhs.baseMipLevel, lhs.layerCount, lhs.levelCount) <
        std::tie(rhs.aspectMask, rhs.baseArrayLayer, rhs.baseMipLevel, rhs.layerCount, rhs.levelCount);
};

class VKDevice;

class VKResource : public ResourceBase
{
public:
    VKResource(VKDevice& device);
    uint64_t GetWidth() const override;
    uint32_t GetHeight() const override;
    uint16_t GetLayerCount() const override;
    uint16_t GetLevelCount() const override;
    uint32_t GetSampleCount() const override;
    uint64_t GetAccelerationStructureHandle() const override;
    void SetName(const std::string& name) override;
    uint8_t* Map() override;
    void Unmap() override;

    struct Image
    {
        vk::UniqueImage res;
        vk::UniqueDeviceMemory memory;
        vk::Format format = vk::Format::eUndefined;
        vk::Extent2D size = {};
        size_t level_count = 1;
        size_t sample_count = 1;
        size_t array_layers = 1;
    } image;

    struct Buffer
    {
        vk::UniqueBuffer res;
        vk::UniqueDeviceMemory memory;
        uint32_t size = 0;
    } buffer;

    struct Sampler
    {
        vk::UniqueSampler res;
    } sampler;

    struct AccelerationStructure
    {
        vk::UniqueDeviceMemory memory;
        vk::UniqueAccelerationStructureNV acceleration_structure;
        uint64_t handle;
    } as;

private:
    VKDevice& m_device;
};

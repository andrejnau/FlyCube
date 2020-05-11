#pragma once
#include "Resource/Resource.h"
#include <glm/glm.hpp>
#include <map>
#include <Utilities/Vulkan.h>

struct AccelerationStructure
{
    vk::UniqueDeviceMemory memory;
    vk::UniqueAccelerationStructureNV accelerationStructure;
    uint64_t handle;

    vk::GeometryNV geometry = {};

    vk::UniqueBuffer scratchBuffer;
    vk::UniqueDeviceMemory scratchmemory;

    vk::UniqueBuffer geometryInstance;
    vk::UniqueDeviceMemory geo_memory;
};

struct GeometryInstance
{
    glm::mat3x4 transform;
    uint32_t instanceId : 24;
    uint32_t mask : 8;
    uint32_t instanceOffset : 24;
    uint32_t flags : 8;
    uint64_t accelerationStructureHandle;
};

static bool operator<(const VkImageSubresourceRange& lhs, const VkImageSubresourceRange& rhs)
{
    return std::tie(lhs.aspectMask, lhs.baseArrayLayer, lhs.baseMipLevel, lhs.layerCount, lhs.levelCount) <
        std::tie(rhs.aspectMask, rhs.baseArrayLayer, rhs.baseMipLevel, rhs.layerCount, rhs.levelCount);
};

class VKDevice;

class VKResource : public Resource
{
public:
    VKResource(VKDevice& device);
    void SetName(const std::string& name) override;

    VKDevice& m_device;

    struct Image
    {
        vk::UniqueImage res;
        vk::UniqueDeviceMemory memory;
        std::map<VkImageSubresourceRange, vk::ImageLayout> layout;
        vk::Format format = vk::Format::eUndefined;
        vk::Extent2D size = {};
        size_t level_count = 1;
        size_t msaa_count = 1;
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

    AccelerationStructure bottom_as;
    AccelerationStructure top_as;

    enum class Type
    {
        kUnknown,
        kBuffer,
        kImage,
        kSampler,
        kBottomLevelAS,
        kTopLevelAS,
    };

    Type res_type = Type::kUnknown;

    std::shared_ptr<VKResource> GetUploadResource(size_t subresource)
    {
        if (subresource >= m_upload_res.size())
            m_upload_res.resize(subresource + 1);
        return m_upload_res[subresource];
    }

private:
    std::vector<std::shared_ptr<VKResource>> m_upload_res;
};

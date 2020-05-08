#pragma once

#include <vulkan/vulkan.hpp>
#include "Resource/Resource.h"

class VKDescriptorHeapRange;

using VKBindKey = std::tuple<size_t /*program_id*/, ShaderType /*shader_type*/, VkDescriptorType /*res_type*/, uint32_t /*slot*/>;

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

class VKContext;

class VKResource
    : public Resource
{
public:
    using Ptr = std::shared_ptr<VKResource>;

    //std::reference_wrapper<VKContext> m_context;

    VKResource();
    //VKResource(VKContext& context);
    VKResource(VKResource&&);
    ~VKResource();

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

    VKResource::Ptr GetUploadResource(size_t subresource)
    {
        if (subresource >= m_upload_res.size())
            m_upload_res.resize(subresource + 1);
        return m_upload_res[subresource];
    }

    virtual void SetName(const std::string& name) override
    {
    }

private:
    std::vector<VKResource::Ptr> m_upload_res;
    bool empty = false;
};

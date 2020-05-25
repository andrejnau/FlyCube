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
    ResourceType GetResourceType() const override;
    gli::format GetFormat() const override;
    MemoryType GetMemoryType() const override;
    uint64_t GetWidth() const override;
    uint32_t GetHeight() const override;
    uint16_t GetDepthOrArraySize() const override;
    uint16_t GetMipLevels() const override;
    void SetName(const std::string& name) override;
    uint8_t* Map() override;
    void Unmap() override;
    void UpdateUploadData(const void* data, uint64_t offset, uint64_t num_bytes) override;
    void UpdateSubresource(uint64_t buffer_offset, uint32_t buffer_row_pitch, uint32_t buffer_depth_pitch,
                           const void* src_data, uint32_t src_row_pitch, uint32_t src_depth_pitch, uint32_t num_rows, uint32_t num_slices) override;

    VKDevice& m_device;
    gli::format m_format;
    MemoryType memory_type = MemoryType::kDefault;

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

    ResourceType res_type = ResourceType::kUnknown;

    std::shared_ptr<VKResource>& GetUploadResource(size_t subresource)
    {
        if (subresource >= m_upload_res.size())
            m_upload_res.resize(subresource + 1);
        return m_upload_res[subresource];
    }

private:
    std::vector<std::shared_ptr<VKResource>> m_upload_res;
};

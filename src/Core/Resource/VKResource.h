#pragma once
#include "Resource/ResourceBase.h"
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

class VKDevice;

class VKResource : public ResourceBase
{
public:
    VKResource(VKDevice& device);
    uint64_t GetWidth() const override;
    uint32_t GetHeight() const override;
    uint16_t GetDepthOrArraySize() const override;
    uint16_t GetMipLevels() const override;
    void SetName(const std::string& name) override;
    uint8_t* Map() override;
    void Unmap() override;

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

private:
    VKDevice& m_device;
};

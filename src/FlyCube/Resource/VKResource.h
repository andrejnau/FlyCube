#pragma once
#include "Resource/ResourceBase.h"
#include "Utilities/Common.h"

#include <vulkan/vulkan.hpp>

class VKDevice;

class VKResource : public ResourceBase {
public:
    VKResource(PassKey<VKResource> pass_key, VKDevice& device);

    static std::shared_ptr<VKResource> WrapSwapchainImage(VKDevice& device,
                                                          vk::Image image,
                                                          gli::format format,
                                                          uint32_t width,
                                                          uint32_t height);

    static std::shared_ptr<VKResource> CreateTexture(VKDevice& device,
                                                     TextureType type,
                                                     uint32_t bind_flag,
                                                     gli::format format,
                                                     uint32_t sample_count,
                                                     int width,
                                                     int height,
                                                     int depth,
                                                     int mip_levels);

    static std::shared_ptr<VKResource> CreateBuffer(VKDevice& device, uint32_t bind_flag, uint32_t buffer_size);

    static std::shared_ptr<VKResource> CreateSampler(VKDevice& device, const SamplerDesc& desc);

    static std::shared_ptr<VKResource> CreateAccelerationStructure(
        VKDevice& device,
        AccelerationStructureType type,
        const std::shared_ptr<Resource>& acceleration_structures_memory,
        uint64_t offset);

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
    MemoryRequirements GetMemoryRequirements() const override;

    const vk::Image& GetImage() const;
    const vk::Buffer& GetBuffer() const;
    const vk::Sampler& GetSampler() const;
    const vk::AccelerationStructureKHR& GetAccelerationStructure() const;

private:
    VKDevice& m_device;
    vk::DeviceMemory m_vk_memory;

    vk::UniqueImage m_image_owned;

    struct Image {
        vk::Image res;
        vk::Extent2D size = {};
        uint32_t level_count = 1;
        uint32_t sample_count = 1;
        uint32_t array_layers = 1;
    } m_image;

    struct Buffer {
        vk::UniqueBuffer res;
        uint32_t size = 0;
    } m_buffer;

    struct Sampler {
        vk::UniqueSampler res;
    } m_sampler;

    vk::UniqueAccelerationStructureKHR m_acceleration_structure = {};
};

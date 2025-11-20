#pragma once
#include "Resource/ResourceBase.h"
#include "Utilities/PassKey.h"

#include <vulkan/vulkan.hpp>

class VKDevice;
class VKMemory;

class VKResource : public ResourceBase {
public:
    VKResource(PassKey<VKResource> pass_key, VKDevice& device);

    static std::shared_ptr<VKResource> WrapSwapchainImage(VKDevice& device,
                                                          vk::Image image,
                                                          const TextureDesc& desc,
                                                          vk::ImageUsageFlags usage);
    static std::shared_ptr<VKResource> CreateImage(VKDevice& device, const TextureDesc& desc);
    static std::shared_ptr<VKResource> CreateBuffer(VKDevice& device, const BufferDesc& desc);
    static std::shared_ptr<VKResource> CreateSampler(VKDevice& device, const SamplerDesc& desc);
    static std::shared_ptr<VKResource> CreateAccelerationStructure(VKDevice& device,
                                                                   const AccelerationStructureDesc& desc);

    void CommitMemory(MemoryType memory_type);
    void BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset);
    uint64_t GetWidth() const override;
    uint32_t GetHeight() const override;
    uint16_t GetLayerCount() const override;
    uint16_t GetLevelCount() const override;
    uint32_t GetSampleCount() const override;
    uint64_t GetAccelerationStructureHandle() const override;
    void SetName(const std::string& name) override;
    uint8_t* Map() override;
    void Unmap() override;
    MemoryRequirements GetMemoryRequirements() const;

    const vk::Image& GetImage() const;
    const vk::Buffer& GetBuffer() const;
    const vk::Sampler& GetSampler() const;
    const vk::AccelerationStructureKHR& GetAccelerationStructure() const;
    vk::ImageCreateFlags GetImageCreateFlags() const;
    vk::ImageUsageFlags GetImageUsageFlags() const;

private:
    VKDevice& device_;

    std::shared_ptr<VKMemory> commited_memory_;
    vk::DeviceMemory vk_memory_;

    vk::UniqueImage image_owned_;

    struct Image {
        vk::Image res;
        TextureDesc desc;
        vk::ImageCreateFlags flags;
        vk::ImageUsageFlags usage;
    } image_;

    struct Buffer {
        vk::UniqueBuffer res;
        uint64_t size = 0;
    } buffer_;

    struct Sampler {
        vk::UniqueSampler res;
    } sampler_;

    vk::UniqueAccelerationStructureKHR acceleration_structure_ = {};
};

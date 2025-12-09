#pragma once
#include "Resource/VKResource.h"
#include "Utilities/PassKey.h"

#include <vulkan/vulkan.hpp>

class VKDevice;
class VKMemory;

class VKTexture : public VKResource {
public:
    VKTexture(PassKey<VKTexture> pass_key, VKDevice& device);

    static std::shared_ptr<VKTexture> WrapSwapchainImage(VKDevice& device, vk::Image image, const TextureDesc& desc);
    static std::shared_ptr<VKTexture> CreateImage(VKDevice& device, const TextureDesc& desc);

    void CommitMemory(MemoryType memory_type);
    void BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset);
    MemoryRequirements GetMemoryRequirements() const;

    // Resource:
    uint64_t GetWidth() const override;
    uint32_t GetHeight() const override;
    uint16_t GetLayerCount() const override;
    uint16_t GetLevelCount() const override;
    uint32_t GetSampleCount() const override;
    void SetName(const std::string& name) override;

    // VKResource:
    vk::Image GetImage() const override;

private:
    VKDevice& device_;

    std::shared_ptr<VKMemory> commited_memory_;
    vk::UniqueImage image_owned_;
    vk::Image image_;
    TextureDesc image_desc_;
};

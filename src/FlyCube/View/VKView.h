#pragma once
#include "Resource/VKResource.h"
#include "View/View.h"

class VKBindlessTypedViewPool;
class VKDevice;

class VKView : public View {
public:
    VKView(VKDevice& device, const std::shared_ptr<VKResource>& resource, const ViewDesc& view_desc);
    std::shared_ptr<Resource> GetResource() override;
    uint32_t GetDescriptorId() const override;
    uint32_t GetBaseMipLevel() const override;
    uint32_t GetLevelCount() const override;
    uint32_t GetBaseArrayLayer() const override;
    uint32_t GetLayerCount() const override;

    vk::ImageView GetImageView() const;
    vk::WriteDescriptorSet GetDescriptor() const;

private:
    void CreateView();
    void CreateImageView();
    void CreateBufferView();

    VKDevice& device_;
    std::shared_ptr<VKResource> resource_;
    ViewDesc view_desc_;
    vk::UniqueImageView image_view_;
    vk::UniqueBufferView buffer_view_;
    vk::DescriptorImageInfo descriptor_image_;
    vk::DescriptorBufferInfo descriptor_buffer_;
    vk::AccelerationStructureKHR acceleration_structure_;
    vk::WriteDescriptorSetAccelerationStructureKHR descriptor_acceleration_structure_;
    vk::WriteDescriptorSet descriptor_;
    std::shared_ptr<VKBindlessTypedViewPool> bindless_view_pool_;
};

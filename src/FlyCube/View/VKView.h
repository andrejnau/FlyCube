#pragma once
#include "GPUDescriptorPool/VKGPUDescriptorPoolRange.h"
#include "Resource/VKResource.h"
#include "View/View.h"

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

    VKDevice& m_device;
    std::shared_ptr<VKResource> m_resource;
    ViewDesc m_view_desc;
    vk::UniqueImageView m_image_view;
    vk::UniqueBufferView m_buffer_view;
    std::shared_ptr<VKGPUDescriptorPoolRange> m_range;
    vk::DescriptorImageInfo m_descriptor_image = {};
    vk::DescriptorBufferInfo m_descriptor_buffer = {};
    vk::WriteDescriptorSetAccelerationStructureKHR m_descriptor_acceleration_structure = {};
    vk::WriteDescriptorSet m_descriptor = {};
};

#pragma once
#include "View/View.h"
#include <Resource/VKResource.h>
#include <GPUDescriptorPool/VKGPUDescriptorPoolRange.h>

class VKDevice;

class VKView : public View
{
public:
    VKView(VKDevice& device, const std::shared_ptr<VKResource>& resource, const ViewDesc& view_desc);
    std::shared_ptr<Resource> GetResource() override;
    uint32_t GetDescriptorId() const override;
    const vk::ImageViewCreateInfo& GeViewInfo() const;
    vk::ImageView GetRtv() const;
    vk::ImageView GetSrv() const;

    void WriteView(vk::WriteDescriptorSet& descriptor_write,
                   std::list<vk::DescriptorImageInfo>& list_image_info,
                   std::list<vk::DescriptorBufferInfo>& list_buffer_info,
                   std::list<vk::WriteDescriptorSetAccelerationStructureNV>& list_as);

private:
    void CreateSrv(const ViewDesc& view_desc, const VKResource& res);
    void CreateRTV(const ViewDesc& view_desc, const VKResource& res);

    VKDevice& m_device;
    std::shared_ptr<VKResource> m_resource;
    ViewDesc m_view_desc;
    vk::ImageViewCreateInfo m_view_info = {};
    vk::UniqueImageView m_srv = {};
    vk::UniqueImageView m_om = {};
    std::shared_ptr<VKGPUDescriptorPoolRange> m_range;
};

#pragma once
#include "View/View.h"
#include <Resource/VKResource.h>

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

private:
    void CreateSrv(const ViewDesc& view_desc, const VKResource& res);
    void CreateRTV(const ViewDesc& view_desc, const VKResource& res);

    VKDevice& m_device;
    std::shared_ptr<VKResource> m_resource;
    vk::ImageViewCreateInfo m_view_info = {};
    vk::UniqueImageView m_srv = {};
    vk::UniqueImageView m_om = {};
};

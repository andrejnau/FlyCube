#pragma once
#include "View/View.h"
#include <Resource/SWResource.h>

class SWDevice;

class SWView : public View
{
public:
    SWView(SWDevice& device, const std::shared_ptr<SWResource>& resource, const ViewDesc& view_desc);
    std::shared_ptr<Resource> GetResource() override;
    uint32_t GetDescriptorId() const override;
    uint32_t GetBaseMipLevel() const override;
    uint32_t GetLevelCount() const override;
    uint32_t GetBaseArrayLayer() const override;
    uint32_t GetLayerCount() const override;

private:
    SWDevice& m_device;
    std::shared_ptr<SWResource> m_resource;
    ViewDesc m_view_desc;
};

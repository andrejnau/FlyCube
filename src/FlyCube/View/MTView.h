#pragma once
#include "Resource/MTResource.h"
#include "View/View.h"

class MTDevice;
class MTResource;

class MTView : public View {
public:
    MTView(MTDevice& device, const std::shared_ptr<MTResource>& resource, const ViewDesc& view_desc);
    std::shared_ptr<Resource> GetResource() override;
    uint32_t GetDescriptorId() const override;
    uint32_t GetBaseMipLevel() const override;
    uint32_t GetLevelCount() const override;
    uint32_t GetBaseArrayLayer() const override;
    uint32_t GetLayerCount() const override;

    void CreateTextureView();
    std::shared_ptr<MTResource> GetMTResource() const;
    id<MTLTexture> GetTextureView() const;
    const ViewDesc& GetViewDesc() const;
    id<MTLResource> GetNativeResource() const;
    uint64_t GetGpuAddress() const;

private:
    MTDevice& m_device;
    std::shared_ptr<MTResource> m_resource;
    ViewDesc m_view_desc;
    id<MTLTexture> m_texture_view = nullptr;
};

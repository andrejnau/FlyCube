#pragma once
#include "Resource/MTResource.h"
#include "View/View.h"

class MTDevice;
class MTResource;
class MTGPUArgumentBufferRange;

class MTView : public View {
public:
    MTView(MTDevice& device, const std::shared_ptr<MTResource>& resource, const ViewDesc& view_desc);
    std::shared_ptr<Resource> GetResource() override;
    uint32_t GetDescriptorId() const override;
    uint32_t GetBaseMipLevel() const override;
    uint32_t GetLevelCount() const override;
    uint32_t GetBaseArrayLayer() const override;
    uint32_t GetLayerCount() const override;

    const ViewDesc& GetViewDesc() const;
    id<MTLResource> GetNativeResource() const;
    uint64_t GetGpuAddress() const;
    id<MTLBuffer> GetBuffer() const;
    id<MTLSamplerState> GetSampler() const;
    id<MTLTexture> GetTexture() const;
    id<MTLAccelerationStructure> GetAccelerationStructure() const;

private:
    void CreateTextureView();

    MTDevice& device_;
    std::shared_ptr<MTResource> resource_;
    ViewDesc view_desc_;
    id<MTLTexture> texture_view_ = nullptr;
    std::shared_ptr<MTGPUArgumentBufferRange> range_;
};

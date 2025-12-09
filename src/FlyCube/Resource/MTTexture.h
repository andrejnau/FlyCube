#pragma once
#include "Resource/MTResource.h"
#include "Utilities/PassKey.h"

#import <Metal/Metal.h>

class MTDevice;

class MTTexture : public MTResource {
public:
    MTTexture(PassKey<MTTexture> pass_key, MTDevice& device);

    static std::shared_ptr<MTTexture> CreateSwapchainTexture(MTDevice& device, const TextureDesc& desc);
    static std::shared_ptr<MTTexture> CreateTexture(MTDevice& device, const TextureDesc& desc);

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

    // MTResource:
    id<MTLTexture> GetTexture() const override;

private:
    MTDevice& device_;

    id<MTLTexture> texture_ = nullptr;
    TextureDesc texture_desc = {};
};

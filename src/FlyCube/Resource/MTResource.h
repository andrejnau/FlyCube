#pragma once
#include "Resource/ResourceBase.h"

#import <Metal/Metal.h>
#include <glm/glm.hpp>

#include <map>

class MTDevice;

class MTResource : public ResourceBase {
public:
    MTResource(MTDevice& device);

    void CommitMemory(MemoryType memory_type) override;
    void BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset) override;
    uint64_t GetWidth() const override;
    uint32_t GetHeight() const override;
    uint16_t GetLayerCount() const override;
    uint16_t GetLevelCount() const override;
    uint32_t GetSampleCount() const override;
    uint64_t GetAccelerationStructureHandle() const override;
    void SetName(const std::string& name) override;
    uint8_t* Map() override;
    void Unmap() override;
    bool AllowCommonStatePromotion(ResourceState state_after) override;
    MemoryRequirements GetMemoryRequirements() const override;

    struct Texture {
        id<MTLTexture> res;
        TextureType type = TextureType::k2D;
        uint32_t bind_flag = 0;
        MTLPixelFormat format = MTLPixelFormatInvalid;
        uint32_t sample_count = 1;
        int width = 1;
        int height = 1;
        int depth = 1;
        int mip_levels = 1;
    } texture;

    struct Buffer {
        id<MTLBuffer> res;
        uint32_t size = 0;
    } buffer;

    struct Sampler {
        id<MTLSamplerState> res;
    } sampler;

    id<MTLAccelerationStructure> acceleration_structure;
    MTLResourceID acceleration_structure_handle = {};

private:
    MTLTextureDescriptor* GetTextureDescriptor(MemoryType memory_type) const;

    MTDevice& m_device;
};

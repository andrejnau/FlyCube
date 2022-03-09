#pragma once
#include "Resource/ResourceBase.h"
#include <glm/glm.hpp>
#include <map>
#import <Metal/Metal.h>

class MTDevice;

class MTResource : public ResourceBase
{
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

    struct Texture
    {
        id<MTLTexture> res;
    } texture;

    struct Buffer
    {
        id<MTLBuffer> res;
        uint32_t size = 0;
    } buffer;

private:
    MTDevice& m_device;
};

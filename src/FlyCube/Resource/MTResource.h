#pragma once
#include "Resource/ResourceBase.h"
#include "Utilities/Common.h"

#import <Metal/Metal.h>
#include <glm/glm.hpp>

class MTDevice;

class MTResource : public ResourceBase {
public:
    MTResource(PassKey<MTResource> pass_key, MTDevice& device);

    static std::shared_ptr<MTResource> CreateSwapchainTexture(MTDevice& device,
                                                              uint32_t bind_flag,
                                                              gli::format format,
                                                              uint32_t width,
                                                              uint32_t height);

    static std::shared_ptr<MTResource> CreateTexture(MTDevice& device,
                                                     TextureType type,
                                                     uint32_t bind_flag,
                                                     gli::format format,
                                                     uint32_t sample_count,
                                                     int width,
                                                     int height,
                                                     int depth,
                                                     int mip_levels);

    static std::shared_ptr<MTResource> CreateBuffer(MTDevice& device, uint32_t bind_flag, uint32_t buffer_size);

    static std::shared_ptr<MTResource> CreateSampler(MTDevice& device, const SamplerDesc& desc);

    static std::shared_ptr<MTResource> CreateAccelerationStructure(MTDevice& device,
                                                                   AccelerationStructureType type,
                                                                   const std::shared_ptr<Resource>& resource,
                                                                   uint64_t offset,
                                                                   uint64_t size);

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
    MemoryRequirements GetMemoryRequirements() const override;

    id<MTLTexture> GetTexture() const;
    id<MTLBuffer> GetBuffer() const;
    id<MTLSamplerState> GetSampler() const;
    id<MTLAccelerationStructure> GetAccelerationStructure() const;

private:
    MTLTextureDescriptor* GetTextureDescriptor(MemoryType memory_type) const;

    MTDevice& m_device;

    struct Texture {
        id<MTLTexture> res = nullptr;
        TextureType type = TextureType::k2D;
        uint32_t bind_flag = 0;
        uint32_t sample_count = 1;
        int width = 1;
        int height = 1;
        int depth = 1;
        int mip_levels = 1;
    } m_texture;

    struct Buffer {
        id<MTLBuffer> res = nullptr;
        uint32_t bind_flag = 0;
        uint32_t size = 0;
    } m_buffer;

    struct Sampler {
        id<MTLSamplerState> res = nullptr;
    } m_sampler;

    id<MTLAccelerationStructure> m_acceleration_structure = nullptr;
};

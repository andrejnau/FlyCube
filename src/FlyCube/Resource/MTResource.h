#pragma once
#include "Resource/ResourceBase.h"
#include "Utilities/PassKey.h"

#import <Metal/Metal.h>
#include <glm/glm.hpp>

class MTDevice;

class MTResource : public ResourceBase {
public:
    MTResource(PassKey<MTResource> pass_key, MTDevice& device);

    static std::shared_ptr<MTResource> CreateSwapchainTexture(MTDevice& device, const TextureDesc& desc);
    static std::shared_ptr<MTResource> CreateTexture(MTDevice& device, const TextureDesc& desc);
    static std::shared_ptr<MTResource> CreateBuffer(MTDevice& device, const BufferDesc& desc);
    static std::shared_ptr<MTResource> CreateSampler(MTDevice& device, const SamplerDesc& desc);
    static std::shared_ptr<MTResource> CreateAccelerationStructure(MTDevice& device,
                                                                   const AccelerationStructureDesc& desc);

    void CommitMemory(MemoryType memory_type);
    void BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset);
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
        TextureDesc desc = {};
    } m_texture;

    struct Buffer {
        id<MTLBuffer> res = nullptr;
        uint32_t bind_flag = 0;
        uint64_t size = 0;
        id<MTLHeap> acceleration_structure_heap = nullptr;
        uint64_t acceleration_structure_heap_offset = 0;
    } m_buffer;

    struct Sampler {
        id<MTLSamplerState> res = nullptr;
    } m_sampler;

    id<MTLAccelerationStructure> m_acceleration_structure = nullptr;
};

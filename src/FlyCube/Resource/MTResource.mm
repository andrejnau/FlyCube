#include "Resource/MTResource.h"

#include "Device/MTDevice.h"
#include "Memory/MTMemory.h"

MTResource::MTResource(PassKey<MTResource> pass_key, MTDevice& device)
    : m_device(device)
{
}

// static
std::shared_ptr<MTResource> MTResource::CreateSwapchainTexture(MTDevice& device,
                                                               uint32_t bind_flag,
                                                               gli::format format,
                                                               uint32_t width,
                                                               uint32_t height)
{
    std::shared_ptr<MTResource> self = std::make_shared<MTResource>(PassKey<MTResource>(), device);
    self->m_resource_type = ResourceType::kTexture;
    self->format = format;
    self->m_is_back_buffer = true;
    self->m_texture = {
        .type = TextureType::k2D,
        .bind_flag = bind_flag,
        .width = static_cast<int>(width),
        .height = static_cast<int>(height),
    };
    self->SetInitialState(ResourceState::kPresent);
    return self;
}

// static
std::shared_ptr<MTResource> MTResource::CreateTexture(MTDevice& device,
                                                      TextureType type,
                                                      uint32_t bind_flag,
                                                      gli::format format,
                                                      uint32_t sample_count,
                                                      int width,
                                                      int height,
                                                      int depth,
                                                      int mip_levels)
{
    std::shared_ptr<MTResource> self = std::make_shared<MTResource>(PassKey<MTResource>(), device);
    self->m_resource_type = ResourceType::kTexture;
    self->format = format;
    self->m_texture = {
        .type = type,
        .bind_flag = bind_flag,
        .sample_count = sample_count,
        .width = width,
        .height = height,
        .depth = depth,
        .mip_levels = mip_levels,
    };
    return self;
}

// static
std::shared_ptr<MTResource> MTResource::CreateBuffer(MTDevice& device, uint32_t bind_flag, uint32_t buffer_size)
{
    if (buffer_size == 0) {
        return nullptr;
    }

    std::shared_ptr<MTResource> self = std::make_shared<MTResource>(PassKey<MTResource>(), device);
    self->m_resource_type = ResourceType::kBuffer;
    self->m_buffer = {
        .size = buffer_size,
    };
    return self;
}

// static
std::shared_ptr<MTResource> MTResource::CreateSampler(MTDevice& device, const SamplerDesc& desc)
{
    MTLSamplerDescriptor* sampler_descriptor = [MTLSamplerDescriptor new];
    sampler_descriptor.supportArgumentBuffers = YES;
    sampler_descriptor.minFilter = MTLSamplerMinMagFilterLinear;
    sampler_descriptor.magFilter = MTLSamplerMinMagFilterLinear;
    sampler_descriptor.mipFilter = MTLSamplerMipFilterLinear;
    sampler_descriptor.maxAnisotropy = 16;
    sampler_descriptor.borderColor = MTLSamplerBorderColorOpaqueBlack;
    sampler_descriptor.lodMinClamp = 0;
    sampler_descriptor.lodMaxClamp = std::numeric_limits<float>::max();

    switch (desc.mode) {
    case SamplerTextureAddressMode::kWrap:
        sampler_descriptor.sAddressMode = MTLSamplerAddressModeRepeat;
        sampler_descriptor.tAddressMode = MTLSamplerAddressModeRepeat;
        sampler_descriptor.rAddressMode = MTLSamplerAddressModeRepeat;
        break;
    case SamplerTextureAddressMode::kClamp:
        sampler_descriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
        sampler_descriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
        sampler_descriptor.rAddressMode = MTLSamplerAddressModeClampToEdge;
        break;
    }

    switch (desc.func) {
    case SamplerComparisonFunc::kNever:
        sampler_descriptor.compareFunction = MTLCompareFunctionNever;
        break;
    case SamplerComparisonFunc::kAlways:
        sampler_descriptor.compareFunction = MTLCompareFunctionAlways;
        break;
    case SamplerComparisonFunc::kLess:
        sampler_descriptor.compareFunction = MTLCompareFunctionLess;
        break;
    }

    std::shared_ptr<MTResource> self = std::make_shared<MTResource>(PassKey<MTResource>(), device);
    self->m_resource_type = ResourceType::kSampler;
    self->m_sampler = {
        .res = [device.GetDevice() newSamplerStateWithDescriptor:sampler_descriptor],
    };
    return self;
}

// static
std::shared_ptr<MTResource> MTResource::CreateAccelerationStructure(MTDevice& device,
                                                                    AccelerationStructureType type,
                                                                    const std::shared_ptr<Resource>& resource,
                                                                    uint64_t offset)
{
    std::shared_ptr<MTResource> self = std::make_shared<MTResource>(PassKey<MTResource>(), device);
    self->m_resource_type = ResourceType::kAccelerationStructure;
    self->m_acceleration_structure =
        [device.GetDevice() newAccelerationStructureWithSize:resource->GetWidth() - offset];
    device.AddAllocationToGlobalResidencySet(self->m_acceleration_structure);
    return self;
}

MTLTextureDescriptor* MTResource::GetTextureDescriptor(MemoryType memory_type) const
{
    MTLTextureDescriptor* texture_descriptor = [MTLTextureDescriptor new];
    switch (m_texture.type) {
    case TextureType::k1D:
        if (m_texture.depth > 1) {
            texture_descriptor.textureType = MTLTextureType1DArray;
        } else {
            texture_descriptor.textureType = MTLTextureType1D;
        }
        break;
    case TextureType::k2D:
        if (m_texture.sample_count > 1) {
            if (m_texture.depth > 1) {
                texture_descriptor.textureType = MTLTextureType2DMultisampleArray;
            } else {
                texture_descriptor.textureType = MTLTextureType2DMultisample;
            }
        } else {
            if (m_texture.depth > 1) {
                texture_descriptor.textureType = MTLTextureType2DArray;
            } else {
                texture_descriptor.textureType = MTLTextureType2D;
            }
        }
        break;
    case TextureType::k3D:
        texture_descriptor.textureType = MTLTextureType3D;
        break;
    }

    MTLTextureUsage usage = {};
    if (m_texture.bind_flag & BindFlag::kRenderTarget) {
        usage |= MTLTextureUsageRenderTarget;
    }
    if (m_texture.bind_flag & BindFlag::kDepthStencil) {
        usage |= MTLTextureUsageRenderTarget;
    }
    if (m_texture.bind_flag & BindFlag::kShaderResource) {
        usage |= MTLTextureUsageShaderRead;
    }
    if (m_texture.bind_flag & BindFlag::kUnorderedAccess) {
        usage |= MTLTextureUsageShaderWrite;
    }

    // TODO: check format
    if (m_texture.bind_flag & BindFlag::kDepthStencil) {
        usage |= MTLTextureUsagePixelFormatView;
    }

    texture_descriptor.pixelFormat = m_device.GetMTLPixelFormat(format);
    texture_descriptor.width = m_texture.width;
    texture_descriptor.height = m_texture.height;
    texture_descriptor.mipmapLevelCount = m_texture.mip_levels;
    texture_descriptor.arrayLength = m_texture.depth;
    texture_descriptor.sampleCount = m_texture.sample_count;
    texture_descriptor.usage = usage;
    texture_descriptor.storageMode = ConvertStorageMode(memory_type);

    return texture_descriptor;
}

void MTResource::CommitMemory(MemoryType memory_type)
{
    m_memory_type = memory_type;
    decltype(auto) mt_device = m_device.GetDevice();
    if (m_resource_type == ResourceType::kBuffer) {
        MTLResourceOptions options = ConvertStorageMode(m_memory_type) << MTLResourceStorageModeShift;
        m_buffer.res = [mt_device newBufferWithLength:m_buffer.size options:options];
        m_device.AddAllocationToGlobalResidencySet(m_buffer.res);
        if (m_buffer.res == nullptr) {
            NSLog(@"Error: failed to create m_buffer");
        }
    } else if (m_resource_type == ResourceType::kTexture) {
        m_texture.res = [mt_device newTextureWithDescriptor:GetTextureDescriptor(m_memory_type)];
        m_device.AddAllocationToGlobalResidencySet(m_texture.res);
        if (m_texture.res == nullptr) {
            NSLog(@"Error: failed to create m_texture");
        }
    }
}

void MTResource::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    m_memory = memory;
    m_memory_type = memory->GetMemoryType();
    id<MTLHeap> mt_heap = m_memory->As<MTMemory>().GetHeap();
    if (m_resource_type == ResourceType::kBuffer) {
        MTLResourceOptions options = ConvertStorageMode(m_memory_type) << MTLResourceStorageModeShift;
        m_buffer.res = [mt_heap newBufferWithLength:m_buffer.size options:options offset:offset];
        m_device.AddAllocationToGlobalResidencySet(m_buffer.res);
        if (m_buffer.res == nullptr) {
            NSLog(@"Error: failed to create m_buffer");
        }
    } else if (m_resource_type == ResourceType::kTexture) {
        m_texture.res = [mt_heap newTextureWithDescriptor:GetTextureDescriptor(m_memory_type) offset:offset];
        m_device.AddAllocationToGlobalResidencySet(m_texture.res);
        if (m_texture.res == nullptr) {
            NSLog(@"Error: failed to create m_texture");
        }
    }
}

uint64_t MTResource::GetWidth() const
{
    if (m_resource_type == ResourceType::kTexture) {
        return m_texture.width;
    }
    return m_buffer.size;
}

uint32_t MTResource::GetHeight() const
{
    return m_texture.height;
}

uint16_t MTResource::GetLayerCount() const
{
    return m_texture.depth;
}

uint16_t MTResource::GetLevelCount() const
{
    return m_texture.mip_levels;
}

uint32_t MTResource::GetSampleCount() const
{
    return m_texture.sample_count;
}

uint64_t MTResource::GetAccelerationStructureHandle() const
{
    return m_acceleration_structure.gpuResourceID._impl;
}

void MTResource::SetName(const std::string& name) {}

uint8_t* MTResource::Map()
{
    if (m_resource_type == ResourceType::kBuffer) {
        return (uint8_t*)m_buffer.res.contents;
    }
    return nullptr;
}

void MTResource::Unmap() {}

MemoryRequirements MTResource::GetMemoryRequirements() const
{
    decltype(auto) mt_device = m_device.GetDevice();
    MTLSizeAndAlign size_align = {};
    if (m_resource_type == ResourceType::kBuffer) {
        MTLResourceOptions options = ConvertStorageMode(MemoryType::kDefault) << MTLResourceStorageModeShift;
        size_align = [mt_device heapBufferSizeAndAlignWithLength:m_buffer.size options:options];
    } else if (m_resource_type == ResourceType::kTexture) {
        size_align = [mt_device heapTextureSizeAndAlignWithDescriptor:GetTextureDescriptor(MemoryType::kDefault)];
    }
    return { size_align.size, size_align.align, 0 };
}

id<MTLTexture> MTResource::GetTexture() const
{
    assert(m_resource_type == ResourceType::kTexture);
    return m_texture.res;
}

id<MTLBuffer> MTResource::GetBuffer() const
{
    assert(m_resource_type == ResourceType::kBuffer);
    return m_buffer.res;
}

id<MTLSamplerState> MTResource::GetSampler() const
{
    assert(m_resource_type == ResourceType::kSampler);
    return m_sampler.res;
}

id<MTLAccelerationStructure> MTResource::GetAccelerationStructure() const
{
    assert(m_resource_type == ResourceType::kAccelerationStructure);
    return m_acceleration_structure;
}

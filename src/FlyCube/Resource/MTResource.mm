#include "Resource/MTResource.h"

#include "Device/MTDevice.h"
#include "Memory/MTMemory.h"
#include "Utilities/Logging.h"

MTResource::MTResource(PassKey<MTResource> pass_key, MTDevice& device)
    : m_device(device)
{
}

// static
std::shared_ptr<MTResource> MTResource::CreateSwapchainTexture(MTDevice& device, const TextureDesc& desc)
{
    std::shared_ptr<MTResource> self = MTResource::CreateTexture(device, desc);
    self->m_is_back_buffer = true;
    self->SetInitialState(ResourceState::kPresent);
    return self;
}

// static
std::shared_ptr<MTResource> MTResource::CreateTexture(MTDevice& device, const TextureDesc& desc)
{
    std::shared_ptr<MTResource> self = std::make_shared<MTResource>(PassKey<MTResource>(), device);
    self->m_resource_type = ResourceType::kTexture;
    self->m_format = desc.format;
    self->m_texture.desc = desc;
    return self;
}

// static
std::shared_ptr<MTResource> MTResource::CreateBuffer(MTDevice& device, const BufferDesc& desc)
{
    if (desc.size == 0) {
        return nullptr;
    }

    std::shared_ptr<MTResource> self = std::make_shared<MTResource>(PassKey<MTResource>(), device);
    self->m_resource_type = ResourceType::kBuffer;
    self->m_buffer = {
        .bind_flag = desc.usage,
        .size = desc.size,
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
                                                                    const AccelerationStructureDesc& desc)
{
    decltype(auto) buffer = desc.buffer->As<MTResource>().m_buffer;
    std::shared_ptr<MTResource> self = std::make_shared<MTResource>(PassKey<MTResource>(), device);
    self->m_resource_type = ResourceType::kAccelerationStructure;
    self->m_acceleration_structure.size = desc.size;
    self->CommitMemory(MemoryType::kDefault);
    return self;
}

MTLTextureDescriptor* MTResource::GetTextureDescriptor(MemoryType memory_type) const
{
    MTLTextureDescriptor* texture_descriptor = [MTLTextureDescriptor new];
    switch (m_texture.desc.type) {
    case TextureType::k1D:
        if (m_texture.desc.depth_or_array_layers > 1) {
            texture_descriptor.textureType = MTLTextureType1DArray;
        } else {
            texture_descriptor.textureType = MTLTextureType1D;
        }
        break;
    case TextureType::k2D:
        if (m_texture.desc.sample_count > 1) {
            if (m_texture.desc.depth_or_array_layers > 1) {
                texture_descriptor.textureType = MTLTextureType2DMultisampleArray;
            } else {
                texture_descriptor.textureType = MTLTextureType2DMultisample;
            }
        } else {
            if (m_texture.desc.depth_or_array_layers > 1) {
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
    if (m_texture.desc.usage & BindFlag::kRenderTarget) {
        usage |= MTLTextureUsageRenderTarget;
    }
    if (m_texture.desc.usage & BindFlag::kDepthStencil) {
        usage |= MTLTextureUsageRenderTarget;
    }
    if (m_texture.desc.usage & BindFlag::kShaderResource) {
        usage |= MTLTextureUsageShaderRead;
    }
    if (m_texture.desc.usage & BindFlag::kUnorderedAccess) {
        usage |= MTLTextureUsageShaderWrite;
    }

    // TODO: check format
    if (m_texture.desc.usage & BindFlag::kDepthStencil) {
        usage |= MTLTextureUsagePixelFormatView;
    }

    texture_descriptor.pixelFormat = m_device.GetMTLPixelFormat(GetFormat());
    texture_descriptor.width = m_texture.desc.width;
    texture_descriptor.height = m_texture.desc.height;
    texture_descriptor.mipmapLevelCount = m_texture.desc.mip_levels;
    texture_descriptor.arrayLength = m_texture.desc.depth_or_array_layers;
    texture_descriptor.sampleCount = m_texture.desc.sample_count;
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
        if (!m_buffer.res) {
            Logging::Println("Failed to create MTLBuffer");
        }
    } else if (m_resource_type == ResourceType::kTexture) {
        m_texture.res = [mt_device newTextureWithDescriptor:GetTextureDescriptor(m_memory_type)];
        if (!m_texture.res) {
            Logging::Println("Failed to create MTLTexture");
        }
    } else if (m_resource_type == ResourceType::kAccelerationStructure) {
        m_acceleration_structure.res = [mt_device newAccelerationStructureWithSize:m_acceleration_structure.size];
        if (!m_acceleration_structure.res) {
            Logging::Println("Failed to create MTLAccelerationStructure");
        }
    }
}

void MTResource::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    m_memory_type = memory->GetMemoryType();
    id<MTLHeap> mt_heap = memory->As<MTMemory>().GetHeap();
    if (m_resource_type == ResourceType::kBuffer) {
        MTLResourceOptions options = ConvertStorageMode(m_memory_type) << MTLResourceStorageModeShift;
        m_buffer.res = [mt_heap newBufferWithLength:m_buffer.size options:options offset:offset];
        if (!m_buffer.res) {
            Logging::Println("Failed to create MTLBuffer from heap {}", mt_heap);
        }
    } else if (m_resource_type == ResourceType::kTexture) {
        m_texture.res = [mt_heap newTextureWithDescriptor:GetTextureDescriptor(m_memory_type) offset:offset];
        if (!m_texture.res) {
            Logging::Println("Failed to create MTLTexture from heap {}", mt_heap);
        }
    } else if (m_resource_type == ResourceType::kAccelerationStructure) {
        m_acceleration_structure.res = [mt_heap newAccelerationStructureWithSize:m_acceleration_structure.size
                                                                          offset:offset];
        if (!m_acceleration_structure.res) {
            Logging::Println("Failed to create MTLAccelerationStructure from heap {}", mt_heap);
        }
    }
}

uint64_t MTResource::GetWidth() const
{
    if (m_resource_type == ResourceType::kTexture) {
        return m_texture.desc.width;
    }
    assert(m_resource_type == ResourceType::kBuffer);
    return m_buffer.size;
}

uint32_t MTResource::GetHeight() const
{
    if (m_resource_type == ResourceType::kTexture) {
        return m_texture.desc.height;
    }
    return 1;
}

uint16_t MTResource::GetLayerCount() const
{
    if (m_resource_type == ResourceType::kTexture) {
        return m_texture.desc.depth_or_array_layers;
    }
    return 1;
}

uint16_t MTResource::GetLevelCount() const
{
    if (m_resource_type == ResourceType::kTexture) {
        return m_texture.desc.mip_levels;
    }
    return 1;
}

uint32_t MTResource::GetSampleCount() const
{
    if (m_resource_type == ResourceType::kTexture) {
        return m_texture.desc.sample_count;
    }
    return 1;
}

uint64_t MTResource::GetAccelerationStructureHandle() const
{
    return GetAccelerationStructure().gpuResourceID._impl;
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
    return m_acceleration_structure.res;
}

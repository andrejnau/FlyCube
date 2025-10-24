#include "Resource/MTResource.h"

#include "Device/MTDevice.h"
#include "Memory/MTMemory.h"

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
    self->m_acceleration_structure = [buffer.acceleration_structure_heap
        newAccelerationStructureWithSize:desc.size
                                  offset:buffer.acceleration_structure_heap_offset + desc.buffer_offset];
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
        if (m_buffer.bind_flag & BindFlag::kAccelerationStructure) {
            MTMemory heap(m_device, m_buffer.size, memory_type);
            m_buffer.acceleration_structure_heap = heap.GetHeap();
            m_buffer.acceleration_structure_heap_offset = 0;
            return;
        }

        MTLResourceOptions options = ConvertStorageMode(m_memory_type) << MTLResourceStorageModeShift;
        m_buffer.res = [mt_device newBufferWithLength:m_buffer.size options:options];
        if (m_buffer.res == nullptr) {
            NSLog(@"Error: failed to create m_buffer");
        }
    } else if (m_resource_type == ResourceType::kTexture) {
        m_texture.res = [mt_device newTextureWithDescriptor:GetTextureDescriptor(m_memory_type)];
        if (m_texture.res == nullptr) {
            NSLog(@"Error: failed to create m_texture");
        }
    }
}

void MTResource::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    m_memory_type = memory->GetMemoryType();
    id<MTLHeap> mt_heap = memory->As<MTMemory>().GetHeap();
    if (m_resource_type == ResourceType::kBuffer) {
        if (m_buffer.bind_flag & BindFlag::kAccelerationStructure) {
            m_buffer.acceleration_structure_heap = mt_heap;
            m_buffer.acceleration_structure_heap_offset = offset;
            return;
        }

        MTLResourceOptions options = ConvertStorageMode(m_memory_type) << MTLResourceStorageModeShift;
        m_buffer.res = [mt_heap newBufferWithLength:m_buffer.size options:options offset:offset];
        if (m_buffer.res == nullptr) {
            NSLog(@"Error: failed to create m_buffer");
        }
    } else if (m_resource_type == ResourceType::kTexture) {
        m_texture.res = [mt_heap newTextureWithDescriptor:GetTextureDescriptor(m_memory_type) offset:offset];
        if (m_texture.res == nullptr) {
            NSLog(@"Error: failed to create m_texture");
        }
    }
}

uint64_t MTResource::GetWidth() const
{
    if (m_resource_type == ResourceType::kTexture) {
        return m_texture.desc.width;
    }
    return m_buffer.size;
}

uint32_t MTResource::GetHeight() const
{
    return m_texture.desc.height;
}

uint16_t MTResource::GetLayerCount() const
{
    return m_texture.desc.depth_or_array_layers;
}

uint16_t MTResource::GetLevelCount() const
{
    return m_texture.desc.mip_levels;
}

uint32_t MTResource::GetSampleCount() const
{
    return m_texture.desc.sample_count;
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

#include "Resource/MTResource.h"

#include "Device/MTDevice.h"
#include "Memory/MTMemory.h"

MTResource::MTResource(MTDevice& device)
    : m_device(device)
{
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
    std::shared_ptr<MTResource> res = std::make_shared<MTResource>(device);
    res->resource_type = ResourceType::kTexture;
    res->format = format;
    res->texture.type = type;
    res->texture.bind_flag = bind_flag;
    res->texture.format = device.GetMVKPixelFormats().getMTLPixelFormat(static_cast<VkFormat>(format));
    res->texture.sample_count = sample_count;
    res->texture.width = width;
    res->texture.height = height;
    res->texture.depth = depth;
    res->texture.mip_levels = mip_levels;
    return res;
}

// static
std::shared_ptr<MTResource> MTResource::CreateBuffer(MTDevice& device, uint32_t bind_flag, uint32_t buffer_size)
{
    if (buffer_size == 0) {
        return {};
    }

    std::shared_ptr<MTResource> res = std::make_shared<MTResource>(device);
    res->resource_type = ResourceType::kBuffer;
    res->buffer.size = buffer_size;
    return res;
}

// static
std::shared_ptr<MTResource> MTResource::CreateSampler(MTDevice& device, const SamplerDesc& desc)
{
    std::shared_ptr<MTResource> res = std::make_shared<MTResource>(device);
    res->resource_type = ResourceType::kSampler;

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

    res->sampler.res = [device.GetDevice() newSamplerStateWithDescriptor:sampler_descriptor];
    return res;
}

// static
std::shared_ptr<MTResource> MTResource::CreateAccelerationStructure(MTDevice& device,
                                                                    AccelerationStructureType type,
                                                                    const std::shared_ptr<Resource>& resource,
                                                                    uint64_t offset)
{
    std::shared_ptr<MTResource> res = std::make_shared<MTResource>(device);
    res->resource_type = ResourceType::kAccelerationStructure;
    res->acceleration_structure = [device.GetDevice() newAccelerationStructureWithSize:resource->GetWidth() - offset];
    device.AddAllocationToGlobalResidencySet(res->acceleration_structure);
    return res;
}

MTLTextureDescriptor* MTResource::GetTextureDescriptor(MemoryType memory_type) const
{
    MTLTextureDescriptor* texture_descriptor = [MTLTextureDescriptor new];
    switch (texture.type) {
    case TextureType::k1D:
        if (texture.depth > 1) {
            texture_descriptor.textureType = MTLTextureType1DArray;
        } else {
            texture_descriptor.textureType = MTLTextureType1D;
        }
        break;
    case TextureType::k2D:
        if (texture.sample_count > 1) {
            if (texture.depth > 1) {
                texture_descriptor.textureType = MTLTextureType2DMultisampleArray;
            } else {
                texture_descriptor.textureType = MTLTextureType2DMultisample;
            }
        } else {
            if (texture.depth > 1) {
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
    if (texture.bind_flag & BindFlag::kRenderTarget) {
        usage |= MTLTextureUsageRenderTarget;
    }
    if (texture.bind_flag & BindFlag::kDepthStencil) {
        usage |= MTLTextureUsageRenderTarget;
    }
    if (texture.bind_flag & BindFlag::kShaderResource) {
        usage |= MTLTextureUsageShaderRead;
    }
    if (texture.bind_flag & BindFlag::kUnorderedAccess) {
        usage |= MTLTextureUsageShaderWrite;
    }

    // TODO: check format
    if (texture.bind_flag & BindFlag::kDepthStencil) {
        usage |= MTLTextureUsagePixelFormatView;
    }

    texture_descriptor.pixelFormat = texture.format;
    texture_descriptor.width = texture.width;
    texture_descriptor.height = texture.height;
    texture_descriptor.mipmapLevelCount = texture.mip_levels;
    texture_descriptor.arrayLength = texture.depth;
    texture_descriptor.sampleCount = texture.sample_count;
    texture_descriptor.usage = usage;
    texture_descriptor.storageMode = ConvertStorageMode(memory_type);

    return texture_descriptor;
}

void MTResource::CommitMemory(MemoryType memory_type)
{
    m_memory_type = memory_type;
    decltype(auto) mt_device = m_device.GetDevice();
    if (resource_type == ResourceType::kBuffer) {
        MTLResourceOptions options = ConvertStorageMode(m_memory_type) << MTLResourceStorageModeShift;
        buffer.res = [mt_device newBufferWithLength:buffer.size options:options];
        m_device.AddAllocationToGlobalResidencySet(buffer.res);
        if (buffer.res == nullptr) {
            NSLog(@"Error: failed to create buffer");
        }
    } else if (resource_type == ResourceType::kTexture) {
        texture.res = [mt_device newTextureWithDescriptor:GetTextureDescriptor(m_memory_type)];
        m_device.AddAllocationToGlobalResidencySet(texture.res);
        if (texture.res == nullptr) {
            NSLog(@"Error: failed to create texture");
        }
    }
}

void MTResource::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    m_memory = memory;
    m_memory_type = memory->GetMemoryType();
    id<MTLHeap> mt_heap = m_memory->As<MTMemory>().GetHeap();
    if (resource_type == ResourceType::kBuffer) {
        MTLResourceOptions options = ConvertStorageMode(m_memory_type) << MTLResourceStorageModeShift;
        buffer.res = [mt_heap newBufferWithLength:buffer.size options:options offset:offset];
        m_device.AddAllocationToGlobalResidencySet(buffer.res);
        if (buffer.res == nullptr) {
            NSLog(@"Error: failed to create buffer");
        }
    } else if (resource_type == ResourceType::kTexture) {
        texture.res = [mt_heap newTextureWithDescriptor:GetTextureDescriptor(m_memory_type) offset:offset];
        m_device.AddAllocationToGlobalResidencySet(texture.res);
        if (texture.res == nullptr) {
            NSLog(@"Error: failed to create texture");
        }
    }
}

uint64_t MTResource::GetWidth() const
{
    if (resource_type == ResourceType::kTexture) {
        return texture.width;
    }
    return buffer.size;
}

uint32_t MTResource::GetHeight() const
{
    return texture.height;
}

uint16_t MTResource::GetLayerCount() const
{
    return texture.depth;
}

uint16_t MTResource::GetLevelCount() const
{
    return texture.mip_levels;
}

uint32_t MTResource::GetSampleCount() const
{
    return texture.sample_count;
}

uint64_t MTResource::GetAccelerationStructureHandle() const
{
    return acceleration_structure.gpuResourceID._impl;
}

void MTResource::SetName(const std::string& name) {}

uint8_t* MTResource::Map()
{
    if (resource_type == ResourceType::kBuffer) {
        return (uint8_t*)buffer.res.contents;
    }
    return nullptr;
}

void MTResource::Unmap() {}

bool MTResource::AllowCommonStatePromotion(ResourceState state_after)
{
    return false;
}

MemoryRequirements MTResource::GetMemoryRequirements() const
{
    decltype(auto) mt_device = m_device.GetDevice();
    MTLSizeAndAlign size_align = {};
    if (resource_type == ResourceType::kBuffer) {
        MTLResourceOptions options = ConvertStorageMode(MemoryType::kDefault) << MTLResourceStorageModeShift;
        size_align = [mt_device heapBufferSizeAndAlignWithLength:buffer.size options:options];
    } else if (resource_type == ResourceType::kTexture) {
        size_align = [mt_device heapTextureSizeAndAlignWithDescriptor:GetTextureDescriptor(MemoryType::kDefault)];
    }
    return { size_align.size, size_align.align, 0 };
}

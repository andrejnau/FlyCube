#include "Resource/MTResource.h"

#include "Device/MTDevice.h"
#include "Memory/MTMemory.h"
#include "Utilities/Logging.h"

MTResource::MTResource(PassKey<MTResource> pass_key, MTDevice& device)
    : device_(device)
{
}

// static
std::shared_ptr<MTResource> MTResource::CreateSwapchainTexture(MTDevice& device, const TextureDesc& desc)
{
    std::shared_ptr<MTResource> self = MTResource::CreateTexture(device, desc);
    self->is_back_buffer_ = true;
    self->SetInitialState(ResourceState::kPresent);
    return self;
}

// static
std::shared_ptr<MTResource> MTResource::CreateTexture(MTDevice& device, const TextureDesc& desc)
{
    std::shared_ptr<MTResource> self = std::make_shared<MTResource>(PassKey<MTResource>(), device);
    self->resource_type_ = ResourceType::kTexture;
    self->format_ = desc.format;
    self->texture_.desc = desc;
    return self;
}

// static
std::shared_ptr<MTResource> MTResource::CreateBuffer(MTDevice& device, const BufferDesc& desc)
{
    if (desc.size == 0) {
        return nullptr;
    }

    std::shared_ptr<MTResource> self = std::make_shared<MTResource>(PassKey<MTResource>(), device);
    self->resource_type_ = ResourceType::kBuffer;
    self->buffer_ = {
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
    self->resource_type_ = ResourceType::kSampler;
    self->sampler_ = {
        .res = [device.GetDevice() newSamplerStateWithDescriptor:sampler_descriptor],
    };
    return self;
}

// static
std::shared_ptr<MTResource> MTResource::CreateAccelerationStructure(MTDevice& device,
                                                                    const AccelerationStructureDesc& desc)
{
    id<MTLAccelerationStructure> acceleration_structure =
        [device.GetDevice() newAccelerationStructureWithSize:desc.size];
    if (!acceleration_structure) {
        Logging::Println("Failed to create MTLAccelerationStructure");
        return nullptr;
    }

    std::shared_ptr<MTResource> self = std::make_shared<MTResource>(PassKey<MTResource>(), device);
    self->resource_type_ = ResourceType::kAccelerationStructure;
    self->acceleration_structure_.res = acceleration_structure;
    return self;
}

MTLTextureDescriptor* MTResource::GetTextureDescriptor(MemoryType memory_type) const
{
    MTLTextureDescriptor* texture_descriptor = [MTLTextureDescriptor new];
    switch (texture_.desc.type) {
    case TextureType::k1D:
        if (texture_.desc.depth_or_array_layers > 1) {
            texture_descriptor.textureType = MTLTextureType1DArray;
        } else {
            texture_descriptor.textureType = MTLTextureType1D;
        }
        break;
    case TextureType::k2D:
        if (texture_.desc.sample_count > 1) {
            if (texture_.desc.depth_or_array_layers > 1) {
                texture_descriptor.textureType = MTLTextureType2DMultisampleArray;
            } else {
                texture_descriptor.textureType = MTLTextureType2DMultisample;
            }
        } else {
            if (texture_.desc.depth_or_array_layers > 1) {
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
    if (texture_.desc.usage & BindFlag::kRenderTarget) {
        usage |= MTLTextureUsageRenderTarget;
    }
    if (texture_.desc.usage & BindFlag::kDepthStencil) {
        usage |= MTLTextureUsageRenderTarget;
    }
    if (texture_.desc.usage & BindFlag::kShaderResource) {
        usage |= MTLTextureUsageShaderRead;
    }
    if (texture_.desc.usage & BindFlag::kUnorderedAccess) {
        usage |= MTLTextureUsageShaderWrite;
    }

    // TODO: check format
    if (texture_.desc.usage & BindFlag::kDepthStencil) {
        usage |= MTLTextureUsagePixelFormatView;
    }

    texture_descriptor.pixelFormat = device_.GetMTLPixelFormat(GetFormat());
    texture_descriptor.width = texture_.desc.width;
    texture_descriptor.height = texture_.desc.height;
    texture_descriptor.mipmapLevelCount = texture_.desc.mip_levels;
    texture_descriptor.arrayLength = texture_.desc.depth_or_array_layers;
    texture_descriptor.sampleCount = texture_.desc.sample_count;
    texture_descriptor.usage = usage;
    texture_descriptor.storageMode = ConvertStorageMode(memory_type);

    return texture_descriptor;
}

void MTResource::CommitMemory(MemoryType memory_type)
{
    memory_type_ = memory_type;
    decltype(auto) mt_device = device_.GetDevice();
    if (resource_type_ == ResourceType::kBuffer) {
        MTLResourceOptions options = ConvertStorageMode(memory_type_) << MTLResourceStorageModeShift;
        buffer_.res = [mt_device newBufferWithLength:buffer_.size options:options];
        if (!buffer_.res) {
            Logging::Println("Failed to create MTLBuffer");
        }
    } else if (resource_type_ == ResourceType::kTexture) {
        texture_.res = [mt_device newTextureWithDescriptor:GetTextureDescriptor(memory_type_)];
        if (!texture_.res) {
            Logging::Println("Failed to create MTLTexture");
        }
    }
}

void MTResource::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    memory_type_ = memory->GetMemoryType();
    id<MTLHeap> mt_heap = memory->As<MTMemory>().GetHeap();
    if (resource_type_ == ResourceType::kBuffer) {
        MTLResourceOptions options = ConvertStorageMode(memory_type_) << MTLResourceStorageModeShift;
        buffer_.res = [mt_heap newBufferWithLength:buffer_.size options:options offset:offset];
        if (!buffer_.res) {
            Logging::Println("Failed to create MTLBuffer from heap {}", mt_heap);
        }
    } else if (resource_type_ == ResourceType::kTexture) {
        texture_.res = [mt_heap newTextureWithDescriptor:GetTextureDescriptor(memory_type_) offset:offset];
        if (!texture_.res) {
            Logging::Println("Failed to create MTLTexture from heap {}", mt_heap);
        }
    }
}

uint64_t MTResource::GetWidth() const
{
    if (resource_type_ == ResourceType::kTexture) {
        return texture_.desc.width;
    }
    assert(resource_type_ == ResourceType::kBuffer);
    return buffer_.size;
}

uint32_t MTResource::GetHeight() const
{
    if (resource_type_ == ResourceType::kTexture) {
        return texture_.desc.height;
    }
    return 1;
}

uint16_t MTResource::GetLayerCount() const
{
    if (resource_type_ == ResourceType::kTexture) {
        return texture_.desc.depth_or_array_layers;
    }
    return 1;
}

uint16_t MTResource::GetLevelCount() const
{
    if (resource_type_ == ResourceType::kTexture) {
        return texture_.desc.mip_levels;
    }
    return 1;
}

uint32_t MTResource::GetSampleCount() const
{
    if (resource_type_ == ResourceType::kTexture) {
        return texture_.desc.sample_count;
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
    if (resource_type_ == ResourceType::kBuffer) {
        return (uint8_t*)buffer_.res.contents;
    }
    return nullptr;
}

void MTResource::Unmap() {}

MemoryRequirements MTResource::GetMemoryRequirements() const
{
    decltype(auto) mt_device = device_.GetDevice();
    MTLSizeAndAlign size_align = {};
    if (resource_type_ == ResourceType::kBuffer) {
        MTLResourceOptions options = ConvertStorageMode(MemoryType::kDefault) << MTLResourceStorageModeShift;
        size_align = [mt_device heapBufferSizeAndAlignWithLength:buffer_.size options:options];
    } else if (resource_type_ == ResourceType::kTexture) {
        size_align = [mt_device heapTextureSizeAndAlignWithDescriptor:GetTextureDescriptor(MemoryType::kDefault)];
    }
    return { size_align.size, size_align.align, 0 };
}

id<MTLTexture> MTResource::GetTexture() const
{
    assert(resource_type_ == ResourceType::kTexture);
    return texture_.res;
}

id<MTLBuffer> MTResource::GetBuffer() const
{
    assert(resource_type_ == ResourceType::kBuffer);
    return buffer_.res;
}

id<MTLSamplerState> MTResource::GetSampler() const
{
    assert(resource_type_ == ResourceType::kSampler);
    return sampler_.res;
}

id<MTLAccelerationStructure> MTResource::GetAccelerationStructure() const
{
    assert(resource_type_ == ResourceType::kAccelerationStructure);
    return acceleration_structure_.res;
}

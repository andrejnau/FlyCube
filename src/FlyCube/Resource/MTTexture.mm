#include "Resource/MTTexture.h"

#include "Device/MTDevice.h"
#include "Memory/MTMemory.h"
#include "Utilities/Logging.h"
#include "Utilities/NotReached.h"

namespace {

MTLTextureDescriptor* GetTextureDescriptor(MTDevice& device, MemoryType memory_type, const TextureDesc& desc)
{
    MTLTextureDescriptor* texture_descriptor = [MTLTextureDescriptor new];
    switch (desc.type) {
    case TextureType::k1D: {
        if (desc.depth_or_array_layers > 1) {
            texture_descriptor.textureType = MTLTextureType1DArray;
        } else {
            texture_descriptor.textureType = MTLTextureType1D;
        }
        break;
    }
    case TextureType::k2D: {
        if (desc.sample_count > 1) {
            if (desc.depth_or_array_layers > 1) {
                texture_descriptor.textureType = MTLTextureType2DMultisampleArray;
            } else {
                texture_descriptor.textureType = MTLTextureType2DMultisample;
            }
        } else {
            if (desc.depth_or_array_layers > 1) {
                texture_descriptor.textureType = MTLTextureType2DArray;
            } else {
                texture_descriptor.textureType = MTLTextureType2D;
            }
        }
        break;
    }
    case TextureType::k3D: {
        texture_descriptor.textureType = MTLTextureType3D;
        break;
    }
    default:
        NOTREACHED();
    }

    MTLTextureUsage usage = {};
    if (desc.usage & BindFlag::kRenderTarget) {
        usage |= MTLTextureUsageRenderTarget;
    }
    if (desc.usage & BindFlag::kDepthStencil) {
        usage |= MTLTextureUsageRenderTarget;
    }
    if (desc.usage & BindFlag::kShaderResource) {
        usage |= MTLTextureUsageShaderRead;
    }
    if (desc.usage & BindFlag::kUnorderedAccess) {
        usage |= MTLTextureUsageShaderWrite;
    }

    // TODO: check format
    if (desc.usage & BindFlag::kDepthStencil) {
        usage |= MTLTextureUsagePixelFormatView;
    }

    texture_descriptor.pixelFormat = device.GetMTLPixelFormat(desc.format);
    texture_descriptor.width = desc.width;
    texture_descriptor.height = desc.height;
    texture_descriptor.mipmapLevelCount = desc.mip_levels;
    texture_descriptor.arrayLength = desc.depth_or_array_layers;
    texture_descriptor.sampleCount = desc.sample_count;
    texture_descriptor.usage = usage;
    texture_descriptor.storageMode = ConvertStorageMode(memory_type);

    return texture_descriptor;
}

} // namespace

MTTexture::MTTexture(PassKey<MTTexture> pass_key, MTDevice& device)
    : device_(device)
{
}

// static
std::shared_ptr<MTTexture> MTTexture::CreateSwapchainTexture(MTDevice& device, const TextureDesc& desc)
{
    std::shared_ptr<MTTexture> self = MTTexture::CreateTexture(device, desc);
    self->is_back_buffer_ = true;
    self->SetInitialState(ResourceState::kPresent);
    return self;
}

// static
std::shared_ptr<MTTexture> MTTexture::CreateTexture(MTDevice& device, const TextureDesc& desc)
{
    std::shared_ptr<MTTexture> self = std::make_shared<MTTexture>(PassKey<MTTexture>(), device);
    self->resource_type_ = ResourceType::kTexture;
    self->format_ = desc.format;
    self->texture_desc = desc;
    return self;
}

void MTTexture::CommitMemory(MemoryType memory_type)
{
    memory_type_ = memory_type;
    MTLTextureDescriptor* texture_descriptor = GetTextureDescriptor(device_, memory_type_, texture_desc);
    texture_ = [device_.GetDevice() newTextureWithDescriptor:texture_descriptor];
    if (!texture_) {
        Logging::Println("Failed to create MTLTexture");
    }
}

void MTTexture::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    memory_type_ = memory->GetMemoryType();
    id<MTLHeap> mt_heap = memory->As<MTMemory>().GetHeap();
    MTLTextureDescriptor* texture_descriptor = GetTextureDescriptor(device_, memory_type_, texture_desc);
    texture_ = [mt_heap newTextureWithDescriptor:texture_descriptor offset:offset];
    if (!texture_) {
        Logging::Println("Failed to create MTLTexture from heap {}", mt_heap);
    }
}

MemoryRequirements MTTexture::GetMemoryRequirements() const
{
    MTLTextureDescriptor* texture_descriptor = GetTextureDescriptor(device_, MemoryType::kDefault, texture_desc);
    MTLSizeAndAlign size_align = [device_.GetDevice() heapTextureSizeAndAlignWithDescriptor:texture_descriptor];
    return { size_align.size, size_align.align, 0 };
}

uint64_t MTTexture::GetWidth() const
{
    return texture_desc.width;
}

uint32_t MTTexture::GetHeight() const
{
    return texture_desc.height;
}

uint16_t MTTexture::GetLayerCount() const
{
    return texture_desc.depth_or_array_layers;
}

uint16_t MTTexture::GetLevelCount() const
{
    return texture_desc.mip_levels;
}

uint32_t MTTexture::GetSampleCount() const
{
    return texture_desc.sample_count;
}

void MTTexture::SetName(const std::string& name)
{
    texture_.label = [NSString stringWithUTF8String:name.c_str()];
}

id<MTLTexture> MTTexture::GetTexture() const
{
    return texture_;
}

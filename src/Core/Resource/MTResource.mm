#include "Resource/MTResource.h"
#include <Device/MTDevice.h>

MTResource::MTResource(MTDevice& device)
    : m_device(device)
{
}

void MTResource::CommitMemory(MemoryType memory_type)
{
    m_memory_type = memory_type;
    decltype(auto) mt_device = m_device.GetDevice();
    if (resource_type == ResourceType::kBuffer)
    {
        MTLResourceOptions options = {};
        buffer.res = [mt_device newBufferWithLength:buffer.size
                                             options:options];
    }
    else if (resource_type == ResourceType::kTexture)
    {
        MTLTextureDescriptor* texture_descriptor = [[MTLTextureDescriptor alloc] init];
        texture_descriptor.pixelFormat = texture.format;
        texture_descriptor.width = texture.width;
        texture_descriptor.height = texture.height;
        texture_descriptor.mipmapLevelCount = texture.mip_levels;
        texture_descriptor.sampleCount = texture.sample_count;
        texture_descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        texture.res = [mt_device newTextureWithDescriptor:texture_descriptor];
    }
}

void MTResource::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    assert(false);
}

uint64_t MTResource::GetWidth() const
{
    if (resource_type == ResourceType::kTexture)
        return texture.width;
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
    return 1;
}

uint64_t MTResource::GetAccelerationStructureHandle() const
{
    return 0;
}

void MTResource::SetName(const std::string& name)
{
}

uint8_t* MTResource::Map()
{
    if (resource_type == ResourceType::kBuffer)
        return (uint8_t*)buffer.res.contents;
    return nullptr;
}

void MTResource::Unmap()
{
}

bool MTResource::AllowCommonStatePromotion(ResourceState state_after)
{
    return false;
}

MemoryRequirements MTResource::GetMemoryRequirements() const
{
    return {};
}

#include "Resource/MTResource.h"
#include <Device/MTDevice.h>

MTResource::MTResource(MTDevice& device)
    : m_device(device)
{
}

void MTResource::CommitMemory(MemoryType memory_type)
{
}

void MTResource::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
}

uint64_t MTResource::GetWidth() const
{
    return 1;
}

uint32_t MTResource::GetHeight() const
{
    return 1;
}

uint16_t MTResource::GetLayerCount() const
{
    return 1;
}

uint16_t MTResource::GetLevelCount() const
{
    return 1;
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

#include "Resource/SWResource.h"

#include "Device/SWDevice.h"
#include "Memory/SWMemory.h"
#include "View/SWView.h"

SWResource::SWResource(SWDevice& device)
    : m_device(device)
{
}

void SWResource::CommitMemory(MemoryType memory_type)
{
    MemoryRequirements mem_requirements = GetMemoryRequirements();
    auto memory = std::make_shared<SWMemory>(mem_requirements.size, memory_type);
    BindMemory(memory, 0);
}

void SWResource::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    m_memory = memory;
    m_offset = offset;
}

uint64_t SWResource::GetWidth() const
{
    return 0;
}

uint32_t SWResource::GetHeight() const
{
    return 0;
}

uint16_t SWResource::GetLayerCount() const
{
    return 0;
}

uint16_t SWResource::GetLevelCount() const
{
    return 0;
}

uint32_t SWResource::GetSampleCount() const
{
    return 0;
}

uint64_t SWResource::GetAccelerationStructureHandle() const
{
    return 0;
}

void SWResource::SetName(const std::string& name) {}

uint8_t* SWResource::Map()
{
    return nullptr;
}

void SWResource::Unmap() {}

bool SWResource::AllowCommonStatePromotion(ResourceState state_after)
{
    return false;
}

MemoryRequirements SWResource::GetMemoryRequirements() const
{
    assert(false);
    return {};
}

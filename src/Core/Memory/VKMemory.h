#pragma once
#include "Memory/Memory.h"
#include <Instance/BaseTypes.h>
#include <Utilities/Vulkan.h>

class VKDevice;

class VKMemory : public Memory
{
public:
    VKMemory(VKDevice& device, uint64_t size, MemoryType memory_type, uint32_t memory_type_bits);
    MemoryType GetMemoryType() const override;
    vk::DeviceMemory GetMemory() const;

private:
    MemoryType m_memory_type;
    vk::UniqueDeviceMemory m_memory;
};

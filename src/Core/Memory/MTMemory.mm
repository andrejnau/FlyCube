#include "Memory/MTMemory.h"
#include <Device/MTDevice.h>

MTMemory::MTMemory(MTDevice& device, uint64_t size, MemoryType memory_type, uint32_t memory_type_bits)
    : m_memory_type(memory_type)
{
}

MemoryType MTMemory::GetMemoryType() const
{
    return m_memory_type;
}

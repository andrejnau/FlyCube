#include "Memory/SWMemory.h"

SWMemory::SWMemory(uint64_t size, MemoryType memory_type)
    : m_memory_type(memory_type)
{
    m_memory.resize(size);
}

MemoryType SWMemory::GetMemoryType() const
{
    return m_memory_type;
}

uint8_t* SWMemory::GetMemory()
{
    return m_memory.data();
}

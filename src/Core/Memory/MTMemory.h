#pragma once
#include "Memory/Memory.h"
#include <Instance/BaseTypes.h>

class MTDevice;

class MTMemory : public Memory
{
public:
    MTMemory(MTDevice& device, uint64_t size, MemoryType memory_type, uint32_t memory_type_bits);
    MemoryType GetMemoryType() const override;

private:
    MemoryType m_memory_type;
};

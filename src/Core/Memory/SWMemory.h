#pragma once
#include "Memory/Memory.h"
#include <Instance/BaseTypes.h>

class SWMemory : public Memory
{
public:
    SWMemory(uint64_t size, MemoryType memory_type);
    MemoryType GetMemoryType() const override;

    uint8_t* GetMemory();

private:
    MemoryType m_memory_type;
    std::vector<uint8_t> m_memory;
};

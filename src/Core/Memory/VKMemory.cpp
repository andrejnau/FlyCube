#include "Memory/VKMemory.h"

#include "Device/VKDevice.h"

VKMemory::VKMemory(VKDevice& device,
                   uint64_t size,
                   MemoryType memory_type,
                   uint32_t memory_type_bits,
                   const vk::MemoryDedicatedAllocateInfoKHR* dedicated_allocate_info)
    : m_memory_type(memory_type)
{
    vk::MemoryAllocateFlagsInfo alloc_flag_info = {};
    alloc_flag_info.pNext = dedicated_allocate_info;
    alloc_flag_info.flags = vk::MemoryAllocateFlagBits::eDeviceAddress;

    vk::MemoryPropertyFlags properties = {};
    if (memory_type == MemoryType::kDefault) {
        properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
    } else if (memory_type == MemoryType::kUpload) {
        properties = vk::MemoryPropertyFlagBits::eHostVisible;
    } else if (memory_type == MemoryType::kReadback) {
        properties = vk::MemoryPropertyFlagBits::eHostVisible;
    }

    vk::MemoryAllocateInfo alloc_info = {};
    alloc_info.pNext = &alloc_flag_info;
    alloc_info.allocationSize = size;
    alloc_info.memoryTypeIndex = device.FindMemoryType(memory_type_bits, properties);
    m_memory = device.GetDevice().allocateMemoryUnique(alloc_info);
}

MemoryType VKMemory::GetMemoryType() const
{
    return m_memory_type;
}

vk::DeviceMemory VKMemory::GetMemory() const
{
    return m_memory.get();
}

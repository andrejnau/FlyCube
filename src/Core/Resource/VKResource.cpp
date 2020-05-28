#include "Resource/VKResource.h"
#include <View/VKView.h>
#include <Device/VKDevice.h>

VKResource::VKResource(VKDevice& device)
    : m_device(device)
{
}

uint64_t VKResource::GetWidth() const
{
    if (resource_type == ResourceType::kTexture)
        return image.size.width;
    return buffer.size;
}

uint32_t VKResource::GetHeight() const
{
    return image.size.height;
}

uint16_t VKResource::GetDepthOrArraySize() const
{
    return image.array_layers;
}

uint16_t VKResource::GetMipLevels() const
{
    return image.level_count;
}

uint64_t VKResource::GetAccelerationStructureHandle() const
{
    return as.handle;
}

void VKResource::SetName(const std::string& name)
{
    vk::DebugUtilsObjectNameInfoEXT info = {};
    info.pObjectName = name.c_str();
    if (resource_type == ResourceType::kBuffer)
    {
        info.objectType = vk::ObjectType::eBuffer;
        info.objectType = buffer.res.get().objectType;
        info.objectHandle = reinterpret_cast<uint64_t>(static_cast<VkBuffer>(buffer.res.get()));
    }
    else if (resource_type == ResourceType::kTexture)
    {
        info.objectType = vk::ObjectType::eBuffer;
        info.objectHandle = reinterpret_cast<uint64_t>(static_cast<VkImage>(image.res.get()));
    }
    m_device.GetDevice().setDebugUtilsObjectNameEXT(info);
}

uint8_t* VKResource::Map()
{
    uint8_t* dst_data = nullptr;
    m_device.GetDevice().mapMemory(buffer.memory.get(), 0, VK_WHOLE_SIZE, {}, reinterpret_cast<void**>(&dst_data));
    return dst_data;
}

void VKResource::Unmap()
{
    m_device.GetDevice().unmapMemory(buffer.memory.get());
}

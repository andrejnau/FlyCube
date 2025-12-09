#include "Resource/VKBuffer.h"

#include "Device/VKDevice.h"
#include "Memory/VKMemory.h"

VKBuffer::VKBuffer(PassKey<VKBuffer> pass_key, VKDevice& device)
    : device_(device)
{
}

// static
std::shared_ptr<VKBuffer> VKBuffer::CreateBuffer(VKDevice& device, const BufferDesc& desc)
{
    if (desc.size == 0) {
        return nullptr;
    }

    vk::BufferCreateInfo buffer_info = {};
    buffer_info.size = desc.size;
    buffer_info.usage = vk::BufferUsageFlagBits::eShaderDeviceAddress;

    if (desc.usage & BindFlag::kVertexBuffer) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eVertexBuffer;
    }
    if (desc.usage & BindFlag::kIndexBuffer) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eIndexBuffer;
    }
    if (desc.usage & BindFlag::kConstantBuffer) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eUniformBuffer;
    }
    if (desc.usage & BindFlag::kUnorderedAccess) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
        buffer_info.usage |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
    }
    if (desc.usage & BindFlag::kShaderResource) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
        buffer_info.usage |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
    }
    if (desc.usage & BindFlag::kAccelerationStructure) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR;
    }
    if (desc.usage & BindFlag::kCopySource) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eTransferSrc;
    }
    if (desc.usage & BindFlag::kCopyDest) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eTransferDst;
    }
    if (desc.usage & BindFlag::kShaderTable) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eShaderBindingTableKHR;
    }
    if (desc.usage & BindFlag::kIndirectBuffer) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eIndirectBuffer;
    }

    std::shared_ptr<VKBuffer> self = std::make_shared<VKBuffer>(PassKey<VKBuffer>(), device);
    self->resource_type_ = ResourceType::kBuffer;
    self->buffer_ = device.GetDevice().createBufferUnique(buffer_info);
    self->buffer_size_ = desc.size;
    self->SetInitialState(ResourceState::kCommon);
    return self;
}

void VKBuffer::CommitMemory(MemoryType memory_type)
{
    MemoryRequirements mem_requirements = GetMemoryRequirements();
    vk::MemoryDedicatedAllocateInfo dedicated_allocate_info = {};
    dedicated_allocate_info.buffer = GetBuffer();
    commited_memory_ = std::make_shared<VKMemory>(device_, mem_requirements.size, memory_type,
                                                  mem_requirements.memory_type_bits, &dedicated_allocate_info);
    BindMemory(commited_memory_, 0);
}

void VKBuffer::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    memory_type_ = memory->GetMemoryType();
    vk_memory_ = memory->As<VKMemory>().GetMemory();
    device_.GetDevice().bindBufferMemory(GetBuffer(), vk_memory_, offset);
}

MemoryRequirements VKBuffer::GetMemoryRequirements() const
{
    vk::MemoryRequirements2 mem_requirements = {};
    vk::BufferMemoryRequirementsInfo2KHR buffer_mem_req = {};
    buffer_mem_req.buffer = GetBuffer();
    device_.GetDevice().getBufferMemoryRequirements2(&buffer_mem_req, &mem_requirements);
    return { mem_requirements.memoryRequirements.size, mem_requirements.memoryRequirements.alignment,
             mem_requirements.memoryRequirements.memoryTypeBits };
}

uint64_t VKBuffer::GetWidth() const
{
    return buffer_size_;
}

void VKBuffer::SetName(const std::string& name)
{
    vk::DebugUtilsObjectNameInfoEXT info = {};
    info.pObjectName = name.c_str();
    info.objectType = GetBuffer().objectType;
    info.objectHandle = reinterpret_cast<uint64_t>(static_cast<VkBuffer>(GetBuffer()));
    device_.GetDevice().setDebugUtilsObjectNameEXT(info);
}

uint8_t* VKBuffer::Map()
{
    uint8_t* dst_data = nullptr;
    std::ignore = device_.GetDevice().mapMemory(vk_memory_, 0, VK_WHOLE_SIZE, {}, reinterpret_cast<void**>(&dst_data));
    return dst_data;
}

void VKBuffer::Unmap()
{
    device_.GetDevice().unmapMemory(vk_memory_);
}

vk::Buffer VKBuffer::GetBuffer() const
{
    return buffer_.get();
}

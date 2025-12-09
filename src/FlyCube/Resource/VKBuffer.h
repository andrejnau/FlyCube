#pragma once
#include "Resource/VKResource.h"
#include "Utilities/PassKey.h"

#include <vulkan/vulkan.hpp>

class VKDevice;
class VKMemory;

class VKBuffer : public VKResource {
public:
    VKBuffer(PassKey<VKBuffer> pass_key, VKDevice& device);

    static std::shared_ptr<VKBuffer> CreateBuffer(VKDevice& device, const BufferDesc& desc);

    void CommitMemory(MemoryType memory_type);
    void BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset);
    MemoryRequirements GetMemoryRequirements() const;

    // Resource:
    uint64_t GetWidth() const override;
    void SetName(const std::string& name) override;
    uint8_t* Map() override;
    void Unmap() override;

    // VKResource:
    vk::Buffer GetBuffer() const override;

private:
    VKDevice& device_;

    std::shared_ptr<VKMemory> commited_memory_;
    vk::DeviceMemory vk_memory_;
    vk::UniqueBuffer buffer_;
    uint64_t buffer_size_ = 0;
};

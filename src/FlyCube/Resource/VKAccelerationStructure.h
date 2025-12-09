#pragma once
#include "Resource/VKResource.h"
#include "Utilities/PassKey.h"

#include <vulkan/vulkan.hpp>

class VKDevice;

class VKAccelerationStructure : public VKResource {
public:
    VKAccelerationStructure(PassKey<VKAccelerationStructure> pass_key, VKDevice& device);

    static std::shared_ptr<VKAccelerationStructure> CreateAccelerationStructure(VKDevice& device,
                                                                                const AccelerationStructureDesc& desc);

    void CommitMemory(MemoryType memory_type);
    void BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset);
    MemoryRequirements GetMemoryRequirements() const;

    // Resource:
    uint64_t GetAccelerationStructureHandle() const override;
    void SetName(const std::string& name) override;

    // VKResource:
    vk::AccelerationStructureKHR GetAccelerationStructure() const override;

private:
    VKDevice& device_;

    vk::UniqueAccelerationStructureKHR acceleration_structure_;
};

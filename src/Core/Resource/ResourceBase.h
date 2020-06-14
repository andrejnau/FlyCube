#pragma once
#include "Resource/Resource.h"

class ResourceBase : public Resource
{
public:
    ResourceBase();

    virtual uint8_t* Map() = 0;
    virtual void Unmap() = 0;

    ResourceType GetResourceType() const override final;
    gli::format GetFormat() const override final;
    MemoryType GetMemoryType() const override final;
    const RaytracingASPrebuildInfo& GetRaytracingASPrebuildInfo() const override final;

    void UpdateUploadData(const void* data, uint64_t offset, uint64_t num_bytes) override final;
    void UpdateSubresource(uint64_t buffer_offset, uint32_t buffer_row_pitch, uint32_t buffer_depth_pitch,
        const void* src_data, uint32_t src_row_pitch, uint32_t src_depth_pitch, uint32_t num_rows, uint32_t num_slices) override final;
    ResourceStateTracker& GetGlobalResourceStateTracker() override final;

    gli::format format = gli::FORMAT_UNDEFINED;
    MemoryType memory_type = MemoryType::kDefault;
    ResourceType resource_type = ResourceType::kUnknown;
    RaytracingASPrebuildInfo prebuild_info = {};

private:
    ResourceStateTracker m_resource_state_tracker;
};

#pragma once
#include "Resource/Resource.h"

class ResourceBase : public Resource {
public:
    ResourceBase();

    ResourceType GetResourceType() const override final;
    gli::format GetFormat() const override final;
    MemoryType GetMemoryType() const override final;

    void UpdateUploadBuffer(uint64_t buffer_offset, const void* data, uint64_t num_bytes) override final;
    void UpdateUploadBufferWithTextureData(uint64_t buffer_offset,
                                           uint32_t buffer_row_pitch,
                                           uint32_t buffer_depth_pitch,
                                           const void* src_data,
                                           uint32_t src_row_pitch,
                                           uint32_t src_depth_pitch,
                                           uint32_t num_rows,
                                           uint32_t num_slices) override final;
    ResourceState GetInitialState() const override final;
    bool IsBackBuffer() const override final;

    void SetInitialState(ResourceState state);

    gli::format format = gli::FORMAT_UNDEFINED;

protected:
    ResourceType m_resource_type = ResourceType::kUnknown;
    std::shared_ptr<Memory> m_memory;
    MemoryType m_memory_type = MemoryType::kDefault;
    bool m_is_back_buffer = false;

private:
    ResourceState m_initial_state = ResourceState::kUnknown;
};

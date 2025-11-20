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
                                           uint64_t buffer_row_pitch,
                                           uint64_t buffer_slice_pitch,
                                           const void* src_data,
                                           uint64_t src_row_pitch,
                                           uint64_t src_slice_pitch,
                                           uint64_t row_size_in_bytes,
                                           uint32_t num_rows,
                                           uint32_t num_slices) override final;
    ResourceState GetInitialState() const override final;
    bool IsBackBuffer() const override final;

    void SetInitialState(ResourceState state);

protected:
    ResourceType resource_type_ = ResourceType::kUnknown;
    gli::format format_ = gli::FORMAT_UNDEFINED;
    MemoryType memory_type_ = MemoryType::kDefault;
    bool is_back_buffer_ = false;

private:
    ResourceState initial_state_ = ResourceState::kCommon;
};

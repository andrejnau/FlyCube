#pragma once
#include "Instance/QueryInterface.h"
#include "Memory/Memory.h"
#include "Resource/ResourceStateTracker.h"
#include "View/View.h"

#include <gli/format.hpp>

#include <memory>
#include <string>

struct MemoryRequirements {
    uint64_t size;
    uint64_t alignment;
    uint32_t memory_type_bits;
};

class Resource : public QueryInterface {
public:
    virtual ~Resource() = default;
    virtual void CommitMemory(MemoryType memory_type) = 0;
    virtual void BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset) = 0;
    virtual ResourceType GetResourceType() const = 0;
    virtual gli::format GetFormat() const = 0;
    virtual MemoryType GetMemoryType() const = 0;
    virtual uint64_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;
    virtual uint16_t GetLayerCount() const = 0;
    virtual uint16_t GetLevelCount() const = 0;
    virtual uint32_t GetSampleCount() const = 0;
    virtual uint64_t GetAccelerationStructureHandle() const = 0;
    virtual void SetName(const std::string& name) = 0;
    virtual uint8_t* Map() = 0;
    virtual void Unmap() = 0;
    virtual void UpdateUploadBuffer(uint64_t buffer_offset, const void* data, uint64_t num_bytes) = 0;
    virtual void UpdateUploadBufferWithTextureData(uint64_t buffer_offset,
                                                   uint32_t buffer_row_pitch,
                                                   uint32_t buffer_depth_pitch,
                                                   const void* src_data,
                                                   uint32_t src_row_pitch,
                                                   uint32_t src_depth_pitch,
                                                   uint32_t num_rows,
                                                   uint32_t num_slices) = 0;
    virtual bool AllowCommonStatePromotion(ResourceState state_after) = 0;
    virtual ResourceState GetInitialState() const = 0;
    virtual MemoryRequirements GetMemoryRequirements() const = 0;
    virtual bool IsBackBuffer() const = 0;
};

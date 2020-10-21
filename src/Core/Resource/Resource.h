#pragma once
#include "Resource/ResourceStateTracker.h"
#include <Instance/QueryInterface.h>
#include <View/View.h>
#include <memory>
#include <string>
#include <gli/format.hpp>

class Resource
    : public QueryInterface
{
public:
    virtual ~Resource() = default;
    virtual ResourceType GetResourceType() const = 0;
    virtual gli::format GetFormat() const = 0;
    virtual MemoryType GetMemoryType() const = 0;
    virtual uint64_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;
    virtual uint16_t GetLayerCount() const = 0;
    virtual uint16_t GetLevelCount() const = 0;
    virtual uint32_t GetSampleCount() const = 0;
    virtual uint64_t GetAccelerationStructureHandle() const = 0;
    virtual const RaytracingASPrebuildInfo& GetRaytracingASPrebuildInfo() const = 0;
    virtual void SetName(const std::string& name) = 0;
    virtual void UpdateUploadBuffer(uint64_t buffer_offset, const void* data, uint64_t num_bytes) = 0;
    virtual void UpdateUploadBufferWithTextureData(uint64_t buffer_offset, uint32_t buffer_row_pitch, uint32_t buffer_depth_pitch,
                                   const void* src_data, uint32_t src_row_pitch, uint32_t src_depth_pitch, uint32_t num_rows, uint32_t num_slices) = 0;
    virtual bool AllowCommonStatePromotion(ResourceState state_after) = 0;
    virtual ResourceState GetInitialState() const = 0;
};

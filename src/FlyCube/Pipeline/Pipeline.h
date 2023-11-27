#pragma once
#include "Instance/BaseTypes.h"
#include "Instance/QueryInterface.h"

#include <memory>

class Pipeline : public QueryInterface {
public:
    virtual ~Pipeline() = default;
    virtual PipelineType GetPipelineType() const = 0;
    virtual std::vector<uint8_t> GetRayTracingShaderGroupHandles(uint32_t first_group, uint32_t group_count) const = 0;
};

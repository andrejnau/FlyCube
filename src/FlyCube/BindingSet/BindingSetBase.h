#pragma once
#include "BindingSet/BindingSet.h"

#include <map>
#include <memory>

class Device;

class BindingSetBase : public BindingSet {
protected:
    void CreateConstantsFallbackBuffer(Device& device, const std::vector<BindingConstants>& constants);
    void UpdateConstantsFallbackBuffer(const std::vector<BindingConstantsData>& constants);

    std::shared_ptr<Resource> fallback_constants_buffer_;
    std::map<BindKey, uint64_t> fallback_constants_buffer_offsets_;
    std::map<BindKey, std::shared_ptr<View>> fallback_constants_buffer_views_;
};

#pragma once
#include "Resource/VKResource.h"
#include "Utilities/PassKey.h"

#include <vulkan/vulkan.hpp>

class VKDevice;

class VKSampler : public VKResource {
public:
    VKSampler(PassKey<VKSampler> pass_key, VKDevice& device);

    static std::shared_ptr<VKSampler> CreateSampler(VKDevice& device, const SamplerDesc& desc);

    // Resource:
    void SetName(const std::string& name) override;

    // VKResource:
    vk::Sampler GetSampler() const override;

private:
    VKDevice& device_;

    vk::UniqueSampler sampler_;
};

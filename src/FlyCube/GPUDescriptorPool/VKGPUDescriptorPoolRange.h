#pragma once
#include <vulkan/vulkan.hpp>

#include <functional>
#include <memory>

class VKGPUBindlessDescriptorPoolTyped;

class VKGPUDescriptorPoolRange {
public:
    VKGPUDescriptorPoolRange(VKGPUBindlessDescriptorPoolTyped& pool, uint32_t offset, uint32_t size);
    vk::DescriptorSet GetDescriptorSet() const;
    uint32_t GetOffset() const;

private:
    std::reference_wrapper<VKGPUBindlessDescriptorPoolTyped> pool_;
    uint32_t offset_;
    uint32_t size_;
    std::unique_ptr<VKGPUDescriptorPoolRange, std::function<void(VKGPUDescriptorPoolRange*)>> callback_;
};

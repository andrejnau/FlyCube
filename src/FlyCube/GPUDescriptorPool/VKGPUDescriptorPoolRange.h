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
    std::reference_wrapper<VKGPUBindlessDescriptorPoolTyped> m_pool;
    uint32_t m_offset;
    uint32_t m_size;
    std::unique_ptr<VKGPUDescriptorPoolRange, std::function<void(VKGPUDescriptorPoolRange*)>> m_callback;
};

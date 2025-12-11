#pragma once
#include "Instance/BaseTypes.h"
#include "View/BindlessTypedViewPool.h"

#include <vulkan/vulkan.hpp>

class View;
class VKDevice;
class VKGPUDescriptorPoolRange;
class VKView;

class VKBindlessTypedViewPool : public BindlessTypedViewPool {
public:
    VKBindlessTypedViewPool(VKDevice& device, ViewType view_type, uint32_t view_count);

    uint32_t GetBaseDescriptorId() const override;
    uint32_t GetViewCount() const override;
    void WriteView(uint32_t index, const std::shared_ptr<View>& view) override;

    void WriteViewImpl(uint32_t index, VKView& view);

private:
    VKDevice& device_;

    uint32_t view_count_;
    vk::DescriptorType descriptor_type_;
    std::shared_ptr<VKGPUDescriptorPoolRange> range_;
};

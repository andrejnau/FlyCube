#include "BindlessTypedViewPool/VKBindlessTypedViewPool.h"

#include "Device/VKDevice.h"
#include "GPUDescriptorPool/VKGPUDescriptorPoolRange.h"
#include "Utilities/Check.h"
#include "View/VKView.h"

VKBindlessTypedViewPool::VKBindlessTypedViewPool(VKDevice& device, ViewType view_type, uint32_t view_count)
    : device_(device)
    , view_count_(view_count)
{
    descriptor_type_ = GetDescriptorType(view_type);
    decltype(auto) pool = device.GetGPUBindlessDescriptorPool(descriptor_type_);
    range_ = std::make_shared<VKGPUDescriptorPoolRange>(pool.Allocate(view_count));
}

uint32_t VKBindlessTypedViewPool::GetBaseDescriptorId() const
{
    return range_->GetOffset();
}

uint32_t VKBindlessTypedViewPool::GetViewCount() const
{
    return view_count_;
}

void VKBindlessTypedViewPool::WriteView(uint32_t index, const std::shared_ptr<View>& view)
{
    WriteViewImpl(index, view->As<VKView>());
}

void VKBindlessTypedViewPool::WriteViewImpl(uint32_t index, VKView& view)
{
    DCHECK(index < view_count_);
    vk::WriteDescriptorSet descriptor = view.GetDescriptor();
    descriptor.dstSet = range_->GetDescriptorSet();
    descriptor.dstBinding = 0;
    descriptor.dstArrayElement = range_->GetOffset() + index;
    descriptor.descriptorCount = 1;
    descriptor.descriptorType = descriptor_type_;
    DCHECK(descriptor.pImageInfo || descriptor.pBufferInfo || descriptor.pTexelBufferView || descriptor.pNext);
    device_.GetDevice().updateDescriptorSets(1, &descriptor, 0, nullptr);
}

#include "View/MTBindlessTypedViewPool.h"

#include "Device/MTDevice.h"
#include "Utilities/Check.h"
#include "View/MTView.h"

MTBindlessTypedViewPool::MTBindlessTypedViewPool(MTDevice& device, ViewType view_type, uint32_t view_count)
    : view_count_(view_count)
{
    decltype(auto) argument_buffer = device.GetBindlessArgumentBuffer();
    range_ = std::make_shared<MTGPUArgumentBufferRange>(argument_buffer.Allocate(view_count));
}

uint32_t MTBindlessTypedViewPool::GetBaseDescriptorId() const
{
    return range_->GetOffset();
}

uint32_t MTBindlessTypedViewPool::GetViewCount() const
{
    return view_count_;
}

void MTBindlessTypedViewPool::WriteView(uint32_t index, const std::shared_ptr<View>& view)
{
    WriteViewImpl(index, view->As<MTView>());
}

void MTBindlessTypedViewPool::WriteViewImpl(uint32_t index, MTView& view)
{
    DCHECK(index < view_count_);
    uint64_t* arguments = static_cast<uint64_t*>(range_->GetArgumentBuffer().contents);
    const uint32_t offset = range_->GetOffset() + index;
    arguments[offset] = view.GetGpuAddress();
    range_->AddAllocation(offset, view.GetNativeResource());
}

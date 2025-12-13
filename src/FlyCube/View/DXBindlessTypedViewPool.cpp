#include "View/DXBindlessTypedViewPool.h"

#include "Device/DXDevice.h"
#include "Utilities/Check.h"
#include "Utilities/NotReached.h"
#include "View/DXView.h"

DXBindlessTypedViewPool::DXBindlessTypedViewPool(DXDevice& device, ViewType view_type, uint32_t view_count)
    : view_count_(view_count)
{
    switch (view_type) {
    case ViewType::kConstantBuffer:
    case ViewType::kTexture:
    case ViewType::kRWTexture:
    case ViewType::kBuffer:
    case ViewType::kRWBuffer:
    case ViewType::kStructuredBuffer:
    case ViewType::kRWStructuredBuffer:
    case ViewType::kByteAddressBuffer:
    case ViewType::kRWByteAddressBuffer:
    case ViewType::kAccelerationStructure: {
        range_ = std::make_shared<DXGPUDescriptorPoolRange>(
            device.GetGPUDescriptorPool().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, view_count));
        break;
    }
    case ViewType::kSampler: {
        range_ = std::make_shared<DXGPUDescriptorPoolRange>(
            device.GetGPUDescriptorPool().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, view_count));
        break;
    }
    default:
        NOTREACHED();
    }
}

uint32_t DXBindlessTypedViewPool::GetBaseDescriptorId() const
{
    return range_->GetOffset();
}

uint32_t DXBindlessTypedViewPool::GetViewCount() const
{
    return view_count_;
}

void DXBindlessTypedViewPool::WriteView(uint32_t index, const std::shared_ptr<View>& view)
{
    WriteViewImpl(index, view->As<DXView>());
}

void DXBindlessTypedViewPool::WriteViewImpl(uint32_t index, DXView& view)
{
    DCHECK(index < view_count_);
    range_->CopyCpuHandle(index, view.GetHandle());
}

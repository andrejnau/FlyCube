#include "Fence/DXFence.h"

#include "Device/DXDevice.h"
#include "Utilities/DXUtility.h"
#include "Utilities/SystemUtils.h"

DXFence::DXFence(DXDevice& device, uint64_t initial_value)
    : device_(device)
{
    CHECK_HRESULT(device.GetDevice()->CreateFence(initial_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));
#if defined(_WIN32)
    fence_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
#endif
}

uint64_t DXFence::GetCompletedValue()
{
    return fence_->GetCompletedValue();
}

void DXFence::Wait(uint64_t value)
{
    if (GetCompletedValue() < value) {
        CHECK_HRESULT(fence_->SetEventOnCompletion(value, fence_event_));
#if defined(_WIN32)
        WaitForSingleObjectEx(fence_event_, INFINITE, FALSE);
#endif
    }
}

void DXFence::Signal(uint64_t value)
{
    fence_->Signal(value);
}

ComPtr<ID3D12Fence> DXFence::GetFence()
{
    return fence_;
}

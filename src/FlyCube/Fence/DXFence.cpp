#include "Fence/DXFence.h"

#include "Device/DXDevice.h"
#include "Utilities/DXUtility.h"
#include "Utilities/SystemUtils.h"

DXFence::DXFence(DXDevice& device, uint64_t initial_value)
    : m_device(device)
{
    CHECK_HRESULT(device.GetDevice()->CreateFence(initial_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
#if defined(_WIN32)
    m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
#endif
}

uint64_t DXFence::GetCompletedValue()
{
    return m_fence->GetCompletedValue();
}

void DXFence::Wait(uint64_t value)
{
    if (GetCompletedValue() < value) {
        CHECK_HRESULT(m_fence->SetEventOnCompletion(value, m_fence_event));
#if defined(_WIN32)
        WaitForSingleObjectEx(m_fence_event, INFINITE, FALSE);
#endif
    }
}

void DXFence::Signal(uint64_t value)
{
    m_fence->Signal(value);
}

ComPtr<ID3D12Fence> DXFence::GetFence()
{
    return m_fence;
}

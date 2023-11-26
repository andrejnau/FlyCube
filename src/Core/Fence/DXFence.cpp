#include "Fence/DXFence.h"

#include "Device/DXDevice.h"
#include "Utilities/DXUtility.h"
#include "Utilities/SystemUtils.h"

#include <directx/d3d12.h>
#include <dxgi1_6.h>

DXFence::DXFence(DXDevice& device, uint64_t initial_value)
    : m_device(device)
{
    ASSERT_SUCCEEDED(device.GetDevice()->CreateFence(initial_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

uint64_t DXFence::GetCompletedValue()
{
    return m_fence->GetCompletedValue();
}

void DXFence::Wait(uint64_t value)
{
    if (GetCompletedValue() < value) {
        ASSERT_SUCCEEDED(m_fence->SetEventOnCompletion(value, m_fence_event));
        WaitForSingleObjectEx(m_fence_event, INFINITE, FALSE);
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

#include "Fence/DXFence.h"
#include <Device/DXDevice.h>
#include <Utilities/DXUtility.h>
#include <Utilities/FileUtility.h>
#include <dxgi1_6.h>
#include <d3d12.h>

DXFence::DXFence(DXDevice& device)
    : m_device(device)
{
    ASSERT_SUCCEEDED(device.GetDevice()->CreateFence(m_fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void DXFence::WaitAndReset()
{
    if (m_fence->GetCompletedValue() < m_fence_value)
    {
        ASSERT_SUCCEEDED(m_fence->SetEventOnCompletion(m_fence_value, m_fence_event));
        WaitForSingleObjectEx(m_fence_event, INFINITE, FALSE);
    }
    ++m_fence_value;
}

ComPtr<ID3D12Fence> DXFence::GetFence()
{
    return m_fence;
}

uint64_t DXFence::GetValue()
{
    return m_fence_value;
}

#include "Semaphore/DXSemaphore.h"

DXSemaphore::DXSemaphore(DXDevice& device)
    : m_fence(device, m_value)
{
}

ComPtr<ID3D12Fence> DXSemaphore::GetFence()
{
    return m_fence.GetFence();
}

uint64_t DXSemaphore::GetValue() const
{
    return m_value;
}

void DXSemaphore::Increment()
{
    ++m_value;
}

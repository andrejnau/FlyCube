#include "Semaphore/DXSemaphore.h"

DXSemaphore::DXSemaphore(DXDevice& device)
    : m_fence(device, FenceFlag::kSignaled)
{
}

DXFence& DXSemaphore::GetFence()
{
    return m_fence;
}

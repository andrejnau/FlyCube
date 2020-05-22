#include "Semaphore/DXSemaphore.h"

DXSemaphore::DXSemaphore(DXDevice& device)
    : m_fence(device)
{
}

DXFence& DXSemaphore::GetFence()
{
    return m_fence;
}

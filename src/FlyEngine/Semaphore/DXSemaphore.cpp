#include "Semaphore/DXSemaphore.h"

void DXSemaphore::Increment()
{
    ++m_fence_value;
}

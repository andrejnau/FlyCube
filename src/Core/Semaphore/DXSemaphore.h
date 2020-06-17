#pragma once
#include "Semaphore/Semaphore.h"
#include <Fence/DXFence.h>

class DXDevice;

class DXSemaphore
    : public Semaphore
{
public:
    DXSemaphore(DXDevice& device);
    ComPtr<ID3D12Fence> GetFence();
    uint64_t GetValue() const;
    void Increment();

private:
    uint64_t m_value = 0;
    DXFence m_fence;
};

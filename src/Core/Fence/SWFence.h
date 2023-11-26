#pragma once
#include "Fence/Fence.h"

class SWDevice;

class SWFence : public Fence
{
public:
    SWFence(SWDevice& device, uint64_t initial_value);
    uint64_t GetCompletedValue() override;
    void Wait(uint64_t value) override;
    void Signal(uint64_t value) override;

private:
    SWDevice& m_device;
};

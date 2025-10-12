#pragma once
#include "Fence/Fence.h"

#import <Metal/Metal.h>

class MTDevice;

class MTFence : public Fence {
public:
    MTFence(MTDevice& device, uint64_t initial_value);
    uint64_t GetCompletedValue() override;
    void Wait(uint64_t value) override;
    void Signal(uint64_t value) override;

    id<MTLSharedEvent> GetSharedEvent();

private:
    MTDevice& m_device;
    id<MTLSharedEvent> m_shared_event = nullptr;
};

#pragma once
#include <cstdint>

class WindowEvents {
public:
    virtual ~WindowEvents() = default;
    virtual void OnResize(uint32_t width, uint32_t height) {}
};

#pragma once
#include <cstdint>

class WindowEvents {
public:
    virtual ~WindowEvents() = default;
    virtual void OnResize(int width, int height) {}
};

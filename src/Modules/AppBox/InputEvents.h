#pragma once
#include <cstdint>

class InputEvents {
public:
    virtual ~InputEvents() = default;
    virtual void OnKey(int key, int action) {}
    virtual void OnMouse(bool first, double xpos, double ypos) {}
    virtual void OnMouseButton(int button, int action) {}
    virtual void OnScroll(double xoffset, double yoffset) {}
    virtual void OnInputChar(uint32_t ch) {}
};

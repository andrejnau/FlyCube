#pragma once

#include <cstdint>

#include "ISample.h"
#include <simple_camera/camera.h>
#include <GLFW/glfw3.h>
#include <state.h>

class SampleBase : public ISample
{
public:
    virtual ~SampleBase() {}

    virtual void OnKey(int key, int action) override
    {
        if (action == GLFW_PRESS)
            keys_[key] = true;
        else if (action == GLFW_RELEASE)
            keys_[key] = false;

        if (key == GLFW_KEY_N && action == GLFW_PRESS)
        {
            auto & state = CurState<bool>::Instance().state;
            state["disable_norm"] = !state["disable_norm"];
        }
    }

    virtual void OnMouse(bool first_event, double xpos, double ypos) override
    {
        if (first_event)
        {
            last_x_ = xpos;
            last_y_ = ypos;
        }

        double xoffset = xpos - last_x_;
        double yoffset = last_y_ - ypos;  // Reversed since y-coordinates go from bottom to left

        last_x_ = xpos;
        last_y_ = ypos;

        camera_.ProcessMouseMovement((float)xoffset, (float)yoffset);
    }

    Camera camera_;
    std::map<int, bool> keys_;
    float last_frame_ = 0.0;
    float delta_time_ = 0.0f;
    double last_x_ = 0.0f;
    double last_y_ = 0.0f;
};

#pragma once

#include "Scene/IScene.h"
#include <Camera/Camera.h>
#include <Geometry/Geometry.h>
#include <glm/glm.hpp>
#include <vector>
#include <map>

class SceneBase : public IScene
{
public:
    virtual ~SceneBase() {}
    virtual void OnKey(int key, int action) override;
    virtual void OnMouse(bool first_event, double xpos, double ypos) override;

protected:
    void UpdateAngle();
    void UpdateCameraMovement();

    Camera m_camera;
    std::map<int, bool> m_keys;
    float m_last_frame = 0.0;
    float m_delta_time = 0.0f;
    double m_last_x = 0.0f;
    double m_last_y = 0.0f;
    float m_angle = 0.0;
};

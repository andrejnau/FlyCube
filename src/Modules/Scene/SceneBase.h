#pragma once

#include "Scene/IScene.h"
#include <SimpleCamera/Camera.h>
#include <Program/IShaderBuffer.h>
#include <Geometry/Geometry.h>
#include <glm/glm.hpp>
#include <vector>
#include <map>

class SceneBase : public IScene
{
public:
    virtual ~SceneBase() {}
    virtual glm::mat4 StoreMatrix(const glm::mat4& x) = 0;
    virtual void OnKey(int key, int action) override;
    virtual void OnMouse(bool first_event, double xpos, double ypos) override;

protected:
    void UpdateAngle();
    void UpdateCameraMovement();
    void UpdateCBuffers(IShaderBuffer& constant_buffer_geometry_pass, IShaderBuffer& constant_buffer_light_pass);
    void SetLight(IShaderBuffer& light_buffer_geometry_pass);
    void SetMaterial(IShaderBuffer& material_buffer_geometry_pass, IMesh& cur_mesh);
    std::vector<int> MapTextures(IMesh& cur_mesh, IShaderBuffer& textures_enables);

    Camera m_camera;
    std::map<int, bool> m_keys;
    float m_last_frame = 0.0;
    float m_delta_time = 0.0f;
    double m_last_x = 0.0f;
    double m_last_y = 0.0f;
    float m_angle = 0.0;
};

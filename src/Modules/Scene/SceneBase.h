#pragma once

#include "Scene/IScene.h"
#include <Camera/Camera.h>
#include <Context/Context.h>
#include <glm/glm.hpp>
#include <vector>
#include <map>

class SceneBase : public IScene
{
public:
    ApiType m_api_type;
    Context::Ptr m_context;
    SceneBase(ApiType api_type)
        : m_api_type(api_type)
    {
    }
    
    virtual ~SceneBase() {}

    static IScene::Ptr Create(ApiType api_type)
    {
        return IScene::Ptr(new SceneBase(api_type));
    }
    //virtual glm::mat4 StoreMatrix(const glm::mat4& x) = 0;
    virtual void OnKey(int key, int action) override;
    virtual void OnMouse(bool first_event, double xpos, double ypos) override;

    virtual void OnInit(int width, int height) override
    {
        m_context = CreareContext(m_api_type, width, height);

        m_shader_geometry_pass = m_context->CreateProgram("shaders/DX11/GeometryPass_VS.hlsl", "shaders/DX11/GeometryPass_PS.hlsl");
        m_shader_light_pass = m_context->CreateProgram("shaders/DX11/LightPass_VS.hlsl", "shaders/DX11/LightPass_PS.hlsl");

        m_model_of_file = m_context->CreateGeometry("model/sponza/sponza.obj");
        m_model_square = m_context->CreateGeometry("model/square.obj");
    }

    virtual void OnSizeChanged(int width, int height) override
    {

    }
    virtual void OnUpdate() override
    {

    }
    virtual void OnRender() override
    {

    }
    virtual void OnDestroy() override
    {

    }

protected:
    Program::Ptr m_shader_geometry_pass;
    Program::Ptr m_shader_light_pass;

    Geometry::Ptr m_model_of_file;
    Geometry::Ptr m_model_square;
   /* void UpdateAngle();
    void UpdateCameraMovement();
    void UpdateCBuffers(IShaderBuffer& constant_buffer_geometry_pass, IShaderBuffer& constant_buffer_light_pass);
    void SetLight(IShaderBuffer& light_buffer_geometry_pass);
    void SetMaterial(IShaderBuffer& material_buffer_geometry_pass, IMesh& cur_mesh);
    std::vector<int> MapTextures(IMesh& cur_mesh, IShaderBuffer& textures_enables);*/

    Camera m_camera;
    std::map<int, bool> m_keys;
    float m_last_frame = 0.0;
    float m_delta_time = 0.0f;
    double m_last_x = 0.0f;
    double m_last_y = 0.0f;
    float m_angle = 0.0;
};

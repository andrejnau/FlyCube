#include <AppBox/AppBox.h>

#pragma once

#include <Scene/SceneBase.h>
#include <Context/Context.h>
#include <Context/ContextSelector.h>
#include <Geometry/Geometry.h>
#include <wrl.h>
#include <Program/Program.h>
#include <ProgramRef/PixelShaderPS.h>
#include <ProgramRef/VertexShaderVS.h>
#include <glm/gtx/transform.hpp>
#include <chrono>
#include <Utilities/State.h>

using namespace Microsoft::WRL;

class Scene : public SceneBase
{
public:
    Scene(ApiType type, GLFWwindow* window, int width, int height)
        : m_width(width)
        , m_height(height)
        , m_context_ptr(CreateContext(type, window, m_width, m_height))
        , m_context(*m_context_ptr)
        , m_program(m_context)
        //, m_model(m_context, "model/sphere.obj")
        //, m_model(m_context, "model/material-ball-in-3d-coat/export3dcoat.obj")
        //, m_model(m_context, "model/knight-artorias/Artorias.fbx")
        //, m_model(m_context, "model/zbrush-for-concept-mech-design-dver/model.dae")
        // , m_model(m_context, "model/sponza_pbr/sponza.obj")
        // , m_model(m_context, "model/knight-artorias/Artorias.obj")
        , m_model(m_context, "model/export3dcoat/export3dcoat.obj")
    {
        InitRT();
    }

    static IScene::Ptr Create(ApiType api_type, GLFWwindow* window, int width, int height)
    {
        return std::make_unique<Scene>(api_type, window, width, height);
    }

    virtual void OnMouse(bool first_event, double xpos, double ypos) override
    {
        if (first_event)
        {
            m_last_x = xpos;
            m_last_y = ypos;
        }

        double xoffset = xpos - m_last_x;
        double yoffset = m_last_y - ypos;

        m_last_x = xpos;
        m_last_y = ypos;

        m_camera.ProcessMouseMovement((float)xoffset, (float)yoffset);
    }

    virtual void OnKey(int key, int action) override
    {
        if (action == GLFW_PRESS)
            m_keys[key] = true;
        else if (action == GLFW_RELEASE)
            m_keys[key] = false;

        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
            CurState::Instance().pause ^= true;
    }

    virtual void OnUpdate() override
    {
        UpdateCameraMovement();

        static std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
        static std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
        static float angle = 0.0;

        end = std::chrono::system_clock::now();
        int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        start = std::chrono::system_clock::now();

        if (CurState::Instance().pause)
            angle += elapsed / 2e6f;

        float z_width = (m_model.bound_box.z_max - m_model.bound_box.z_min);
        float y_width = (m_model.bound_box.y_max - m_model.bound_box.y_min);
        float x_width = (m_model.bound_box.y_max - m_model.bound_box.y_min);
        float model_width = (z_width + y_width + x_width) / 3.0f;
        float scale = 0.1f;// *256.0f / std::max(z_width, x_width);
        model_width *= scale;

        float offset_x = (m_model.bound_box.x_max + m_model.bound_box.x_min) / 2.0f;
        float offset_y = (m_model.bound_box.y_max + m_model.bound_box.y_min) / 2.0f;
        float offset_z = (m_model.bound_box.z_max + m_model.bound_box.z_min) / 2.0f;

        glm::vec3 cameraPosition = glm::vec3(0.0f, model_width * 0.25f, model_width * 3.0f);

        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::mat4 model_mat = /*glm::rotate(-angle, glm::vec3(0.0, 1.0, 0.0)) * glm::translate(glm::vec3(-offset_x, -offset_y, -offset_z)) **/ glm::scale(glm::vec3(scale, scale, scale));
        glm::mat4 view = glm::lookAtRH(cameraPosition, cameraTarget, cameraUp);
        glm::mat4 proj = glm::perspectiveFovRH(45.0f * (3.14f / 180.0f), (float)m_width, (float)m_height, 0.1f, 1024.0f);

        {
            static bool is = true;
            if (is)
            {
                m_camera.SetCameraPos(cameraPosition);
                is = false;
            }
            glm::mat4 tmp;
            m_camera.GetMatrix(proj, view, tmp);
            cameraPosition = m_camera.GetCameraPos();
        }

        m_program.vs.cbuffer.ConstantBuf.model = glm::transpose(model_mat);
        m_program.vs.cbuffer.ConstantBuf.view = glm::transpose(view);
        m_program.vs.cbuffer.ConstantBuf.projection = glm::transpose(proj);

        float light_r = 2.5;
        glm::vec3 light_pos = glm::vec3(light_r * cos(angle), 25.0f, light_r * sin(angle));

        m_program.vs.cbuffer.ConstantBuf.lightPos = glm::vec4(cameraPosition, 0.0);
        //m_program.vs.cbuffer.ConstantBuf.lightPos = glm::vec4(light_pos, 0.0);
        m_program.vs.cbuffer.ConstantBuf.viewPos = glm::vec4(cameraPosition, 0.0);

        size_t cnt = 0;
        for (auto& cur_mesh : m_model.ia.ranges)
            ++cnt;
        m_program.SetMaxEvents(cnt);
    }

    virtual void OnRender() override
    {
        m_render_target_view = m_context.GetBackBuffer();

        m_context.SetViewport(m_width, m_height);

        m_program.UseProgram();

        m_program.ps.sampler.g_sampler.Attach({
            SamplerFilter::kAnisotropic,
            SamplerTextureAddressMode::kWrap,
            SamplerComparisonFunc::kNever });

        float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
        m_program.ps.om.rtv0.Attach(m_render_target_view).Clear(color);
        m_program.ps.om.dsv.Attach(m_depth_stencil_view).Clear(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        m_model.ia.indices.Bind();
        m_model.ia.positions.BindToSlot(m_program.vs.ia.POSITION);
        m_model.ia.normals.BindToSlot(m_program.vs.ia.NORMAL);
        m_model.ia.texcoords.BindToSlot(m_program.vs.ia.TEXCOORD);
        m_model.ia.tangents.BindToSlot(m_program.vs.ia.TANGENT);

        for (auto& range : m_model.ia.ranges)
        {
            auto& material = m_model.GetMaterial(range.id);

            m_program.ps.srv.normalMap.Attach(material.texture.normal);
            m_program.ps.srv.diffuseMap.Attach(material.texture.diffuse);
            m_program.ps.srv.glossMap.Attach(material.texture.shininess);
            m_program.ps.srv.metalnessMap.Attach(material.texture.metalness);
            m_program.ps.srv.aoMap.Attach(material.texture.ao);
            m_program.ps.srv.alphaMap.Attach(material.texture.alpha);

            m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }

        m_context.Present(m_render_target_view);
    }

    virtual void OnResize(int width, int height) override
    {
        m_width = width;
        m_height = height;

        m_render_target_view.reset();
        m_context.OnResize(width, height);

        InitRT();
    }

private:
    void InitRT()
    {
        m_depth_stencil_view = m_context.CreateTexture((BindFlag)(BindFlag::kDsv), DXGI_FORMAT_D24_UNORM_S8_UINT, 1, m_width, m_height, 1);
        m_camera.SetViewport(m_width, m_height);
    }

    Resource::Ptr m_render_target_view;
    Resource::Ptr m_depth_stencil_view;

    int m_width;
    int m_height;
    std::unique_ptr<Context> m_context_ptr;
    Context& m_context;

    Program<PixelShaderPS, VertexShaderVS> m_program;
    Model m_model;
};

int main(int argc, char *argv[])
{
    ApiType type = ApiType::kDX12;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == "--dx11")
            type = ApiType::kDX11;
        else if (arg == "--dx12")
            type = ApiType::kDX12;
    }
    std::string title;
    switch (type)
    {
    case ApiType::kDX11:
        title = "[DX11] testApp";
        break;
    case ApiType::kDX12:
        title = "[DX12] testApp";
        break;
    }
    return AppBox(Scene::Create, type, title, 1280, 720).Run();
}

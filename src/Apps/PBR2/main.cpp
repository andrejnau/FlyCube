#include <AppBox/AppBox.h>
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
#include <WinConsole/WinConsole.h>

class Scene : public SceneBase
{
public:
    Scene(ApiType type, GLFWwindow* window, int width, int height)
        : m_width(width)
        , m_height(height)
        , m_context_ptr(CreateContext(type, window, m_width, m_height))
        , m_context(*m_context_ptr)
        , m_program(m_context)
    {
        m_scene_list.emplace_back(m_context, "model/sponza_pbr/sponza.obj");
        m_scene_list.back().matrix = glm::scale(glm::vec3(0.01f));
        m_scene_list.emplace_back(m_context, "model/export3dcoat/export3dcoat.obj");
        m_scene_list.back().matrix = glm::scale(glm::vec3(0.07f)) * glm::translate(glm::vec3(0.0f, 75.0f, 0.0f)) * glm::rotate(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        m_camera.SetCameraPos(glm::vec3(-3.0, 2.75, 0.0));
        m_camera.SetCameraYaw(-1.75f);

        InitRT();
        m_sampler = m_context.CreateSampler({
            SamplerFilter::kAnisotropic,
            SamplerTextureAddressMode::kWrap,
            SamplerComparisonFunc::kNever });
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

        static float angle = 0.0f;
        static std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
        std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
        int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        start = end;

        if (CurState::Instance().pause)
            angle += elapsed / 2e6f;

        float light_r = 2.5;
        glm::vec3 light_pos = glm::vec3(light_r * cos(angle), 25.0f, light_r * sin(angle));

        glm::vec3 cameraPosition = m_camera.GetCameraPos();

        m_program.ps.cbuffer.light.viewPos = glm::vec4(cameraPosition, 0.0);
        int i = 0;
        for (int x = -13; x <= 13; ++x)
        {
            int q = 1;
            for (int z = -1; z <= 1; ++z)
            {
                if (i < std::size(m_program.ps.cbuffer.light.light_pos))
                {
                    m_program.ps.cbuffer.light.light_pos[i] = glm::vec4(x, 1.5, z - 0.33, 0);
                    float black = 0.0;
                    m_program.ps.cbuffer.light.light_color[i] = glm::vec4(q == 1 ? 1 : black, q == 2 ? 1 : black, q == 3 ? 1 : black, 0.0);
                    ++i;
                    ++q;
                }
            }
        }

        glm::mat4 projection, view, model;
        m_camera.GetMatrix(projection, view, model);

        m_program.vs.cbuffer.ConstantBuf.view = glm::transpose(view);
        m_program.vs.cbuffer.ConstantBuf.projection = glm::transpose(projection);        
    }

    virtual void OnRender() override
    {
        m_render_target_view = m_context.GetBackBuffer();
        m_camera.SetViewport(m_width, m_height);
        m_context.SetViewport(m_width, m_height);

        m_program.UseProgram();

        m_program.ps.sampler.g_sampler.Attach(m_sampler);

        float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
        m_program.ps.om.rtv0.Attach(m_render_target_view).Clear(color);
        m_program.ps.om.dsv.Attach(m_depth_stencil_view).Clear(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        for (auto& model : m_scene_list)
        {
            m_program.vs.cbuffer.ConstantBuf.model = glm::transpose(model.matrix);

            model.ia.indices.Bind();
            model.ia.positions.BindToSlot(m_program.vs.ia.POSITION);
            model.ia.normals.BindToSlot(m_program.vs.ia.NORMAL);
            model.ia.texcoords.BindToSlot(m_program.vs.ia.TEXCOORD);
            model.ia.tangents.BindToSlot(m_program.vs.ia.TANGENT);

            for (auto& range : model.ia.ranges)
            {
                auto& material = model.GetMaterial(range.id);

                m_program.ps.srv.normalMap.Attach(material.texture.normal);
                m_program.ps.srv.albedoMap.Attach(material.texture.diffuse);
                m_program.ps.srv.roughnessMap.Attach(material.texture.shininess);
                m_program.ps.srv.metalnessMap.Attach(material.texture.metalness);
                m_program.ps.srv.aoMap.Attach(material.texture.ao);
                m_program.ps.srv.alphaMap.Attach(material.texture.alpha);

                m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
            }
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
        m_depth_stencil_view = m_context.CreateTexture((BindFlag)(BindFlag::kDsv), gli::format::FORMAT_D24_UNORM_S8_UINT_PACK32, 1, m_width, m_height, 1);
        m_camera.SetViewport(m_width, m_height);
    }

    Resource::Ptr m_render_target_view;
    Resource::Ptr m_depth_stencil_view;
    Resource::Ptr m_sampler;

    int m_width;
    int m_height;
    std::unique_ptr<Context> m_context_ptr;
    Context& m_context;

    Program<PixelShaderPS, VertexShaderVS> m_program;
    SceneModels m_scene_list;
};

int main(int argc, char *argv[])
{
    WinConsole cmd;
    ApiType type = ApiType::kDX11;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == "--dx11")
            type = ApiType::kDX11;
        else if (arg == "--dx12")
            type = ApiType::kDX12;
        else if (arg == "--vk")
            type = ApiType::kVulkan;
        else if (arg == "--gl")
            type = ApiType::kOpenGL;
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
    case ApiType::kVulkan:
        title = "[Vulkan] testApp";
        break;
    case ApiType::kOpenGL:
        title = "[OpenGL] testApp";
        break;
    }

    return AppBox(Scene::Create, type, title, 1280, 720).Run();
}

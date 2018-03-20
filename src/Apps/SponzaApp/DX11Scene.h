#pragma once

#include <Scene/SceneBase.h>
#include <Context/DX11Context.h>
#include <Geometry/Geometry.h>
#include <d3d11.h>
#include <DXGI1_4.h>
#include <wrl.h>
#include <string>

#include <Program/Program.h>
#include <ProgramRef/GeometryPassPS.h>
#include <ProgramRef/GeometryPassVS.h>
#include <ProgramRef/LightPassPS.h>
#include <ProgramRef/LightPassVS.h>

#include "GeometryPass.h"
#include "LightPass.h"
#include "ImGuiPass.h"
#include "ShadowPass.h"
#include "SSAOPass.h"
#include "ComputeLuminance.h"

using namespace Microsoft::WRL;

class DX11Scene : public SceneBase
{
public:
    DX11Scene(ApiType type, GLFWwindow* window, int width, int height);
    ~DX11Scene();

    static IScene::Ptr Create(ApiType api_type, GLFWwindow* window, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;

    virtual void OnKey(int key, int action) override;
    virtual void OnMouse(bool first, double xpos, double ypos) override;
    virtual void OnMouseButton(int button, int action) override;
    virtual void OnScroll(double xoffset, double yoffset) override;
    virtual void OnInputChar(unsigned int ch) override;
    virtual void OnModifySettings(const Settings& settings) override;

private:
    void CreateRT();

    int m_width;
    int m_height;

    std::unique_ptr<Context> m_context_ptr;
    Context& m_context;

    Resource::Ptr m_render_target_view;
    Resource::Ptr m_depth_stencil_view;

    glm::vec3 light_pos;

    SceneModels m_scene_list;
    Model m_model_square;
    GeometryPass m_geometry_pass;
    ShadowPass m_shadow_pass;
    SSAOPass m_ssao_pass;
    LightPass m_light_pass;
    ComputeLuminance m_compute_luminance;
    ImGuiPass m_imgui_pass;
};

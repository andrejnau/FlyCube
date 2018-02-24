#pragma once

#include <Scene/SceneBase.h>
#include <Context/DX11Context.h>
#include <Geometry/DX11Geometry.h>
#include <d3d11.h>
#include <DXGI1_4.h>
#include <wrl.h>
#include <string>

#include <Program/Program.h>
#include <ProgramRef/GeometryPassPS.h>
#include <ProgramRef/GeometryPassVS.h>
#include <ProgramRef/LightPassPS.h>
#include <ProgramRef/LightPassVS.h>

#include "D3D11/GeometryPass.h"
#include "D3D11/LightPass.h"
#include "D3D11/ImGuiPass.h"
#include "D3D11/ShadowPass.h"
#include "D3D11/SSAOPass.h"
#include "D3D11/ComputeLuminance.h"

using namespace Microsoft::WRL;

class DX11Scene : public SceneBase
{
public:
    DX11Scene(GLFWwindow* window, int width, int height);

    static IScene::Ptr Create(GLFWwindow* window, int width, int height);

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

    ComPtr<IUnknown> m_render_target_view;
    ComPtr<IUnknown> m_depth_stencil_view;

    glm::vec3 light_pos;

    int m_width;
    int m_height;
    DX11Context m_context;
    SceneList<DX11Mesh> m_scene_list;
    Model<DX11Mesh> m_model_square;
    GeometryPass m_geometry_pass;
    ShadowPass m_shadow_pass;
    SSAOPass m_ssao_pass;
    LightPass m_light_pass;
    ComputeLuminance m_compute_luminance;
    ImGuiPass m_imgui_pass;
};

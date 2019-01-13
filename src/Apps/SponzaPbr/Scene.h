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
#include "SSAOPass.h"
#include "ComputeLuminance.h"
#include "Settings.h"
#include "IrradianceConversion.h"
#include "BackgroundPass.h"
#include "ShadowPass.h"
#include "IBLCompute.h"
#include "BRDFGen.h"
#include "Equirectangular2Cubemap.h"

using namespace Microsoft::WRL;

class Scene : public SceneBase, public IModifySettings
{
public:
    Scene(ApiType type, GLFWwindow* window, int width, int height);
    ~Scene();

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
    Resource::Ptr m_equirectangular_environment;

    glm::vec3 m_light_pos;

    SceneModels m_scene_list;
    Model m_model_square;
    Model m_model_cube;
    GeometryPass m_geometry_pass;
    ShadowPass m_shadow_pass;
    SSAOPass m_ssao_pass;
    BRDFGen m_brdf;
    Equirectangular2Cubemap m_equirectangular2cubemap;
    IBLCompute m_ibl_compute;
    size_t m_ibl_count = 1;
    std::vector<std::unique_ptr<IrradianceConversion>> m_irradiance_conversion;
    Resource::Ptr m_irradince;
    Resource::Ptr m_prefilter;
    Resource::Ptr m_depth_stencil_view_irradince;
    Resource::Ptr m_depth_stencil_view_prefilter;
    LightPass m_light_pass;
    BackgroundPass m_background_pass;
    ComputeLuminance m_compute_luminance;
    ImGuiPass m_imgui_pass;
    Settings m_settings;
    size_t m_irradince_texture_size = 16;
    size_t m_prefilter_texture_size = 512;
};

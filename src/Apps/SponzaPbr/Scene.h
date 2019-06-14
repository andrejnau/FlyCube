#pragma once

#include <Scene/SceneBase.h>
#include <Context/Context.h>
#include <Geometry/Geometry.h>
#include <string>

#include <Program/Program.h>
#include <ProgramRef/GeometryPassPS.h>
#include <ProgramRef/GeometryPassVS.h>
#include <ProgramRef/LightPassPS.h>
#include <ProgramRef/LightPassVS.h>

#include "SkinningPass.h"
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

#ifdef RAYTRACING_SUPPORT
#include "RayTracingAOPass.h"
#endif

class Scene : public SceneBase, public IModifySettings
{
public:
    Scene(Context& context, int width, int height);
    ~Scene();

    static IScene::Ptr Create(Context& context, int width, int height);

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

    Context& m_context;

    int m_width;
    int m_height;

    Resource::Ptr m_render_target_view;
    Resource::Ptr m_depth_stencil_view;
    Resource::Ptr m_equirectangular_environment;

    glm::vec3 m_light_pos;

    SceneModels m_scene_list;
    Model m_model_square;
    Model m_model_cube;
    SkinningPass m_skinning_pass;
    GeometryPass m_geometry_pass;
    ShadowPass m_shadow_pass;
    SSAOPass m_ssao_pass;
    Resource::Ptr* m_rtao = nullptr;
#ifdef RAYTRACING_SUPPORT
    std::unique_ptr<RayTracingAOPass> m_ray_tracing_ao_pass;
#endif
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

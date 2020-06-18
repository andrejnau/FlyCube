#pragma once
#include <Context/Context.h>
#include <Geometry/Geometry.h>
#include <string>

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
#include "SponzaSettings.h"
#include "IrradianceConversion.h"
#include "BackgroundPass.h"
#include "ShadowPass.h"
#include "IBLCompute.h"
#include "BRDFGen.h"
#include "Equirectangular2Cubemap.h"

#ifdef RAYTRACING_SUPPORT
#include "RayTracingAOPass.h"
#endif

#include <Camera/Camera.h>
#include <glm/glm.hpp>
#include <vector>
#include <map>

class Scene
    : public InputEvents
    , public WindowEvents
    , public IModifySponzaSettings
{
public:
    Scene(const Settings& settings, GLFWwindow* window, int width, int height);
    ~Scene();

    Context& GetContext()
    {
        return m_context;
    }

    void RenderFrame();

    void OnResize(int width, int height) override;

    virtual void OnKey(int key, int action) override;
    virtual void OnMouse(bool first, double xpos, double ypos) override;
    virtual void OnMouseButton(int button, int action) override;
    virtual void OnScroll(double xoffset, double yoffset) override;
    virtual void OnInputChar(unsigned int ch) override;
    virtual void OnModifySponzaSettings(const SponzaSettings& settings) override;

private:
    void CreateRT();

    Context m_context;
    Device& m_device;

    int m_width;
    int m_height;
    std::shared_ptr<CommandListBox> m_upload_command_list;
    std::shared_ptr<Resource> m_render_target_view;
    std::shared_ptr<Resource> m_depth_stencil_view;
    std::shared_ptr<Resource> m_equirectangular_environment;

    glm::vec3 m_light_pos;

    SceneModels m_scene_list;
    Model m_model_square;
    Model m_model_cube;
    SkinningPass m_skinning_pass;
    GeometryPass m_geometry_pass;
    ShadowPass m_shadow_pass;
    SSAOPass m_ssao_pass;
    std::shared_ptr<Resource>* m_rtao = nullptr;
#ifdef RAYTRACING_SUPPORT
    std::unique_ptr<RayTracingAOPass> m_ray_tracing_ao_pass;
#endif
    BRDFGen m_brdf;
    Equirectangular2Cubemap m_equirectangular2cubemap;
    IBLCompute m_ibl_compute;
    size_t m_ibl_count = 1;
    std::vector<std::unique_ptr<IrradianceConversion>> m_irradiance_conversion;
    std::shared_ptr<Resource> m_irradince;
    std::shared_ptr<Resource> m_prefilter;
    std::shared_ptr<Resource> m_depth_stencil_view_irradince;
    std::shared_ptr<Resource> m_depth_stencil_view_prefilter;
    BackgroundPass m_background_pass;
    LightPass m_light_pass;
    ComputeLuminance m_compute_luminance;
    ImGuiPass m_imgui_pass;
    SponzaSettings m_settings;
    size_t m_irradince_texture_size = 16;
    size_t m_prefilter_texture_size = 512;
    struct PassDesc
    {
        std::string name;
        std::reference_wrapper<IPass> pass;
    };
    std::vector<PassDesc> m_passes;
    std::vector<std::shared_ptr<CommandListBox>> m_command_lists;
    size_t m_command_list_index = 0;

    void UpdateCameraMovement();

    Camera m_camera;
    std::map<int, bool> m_keys;
    float m_last_frame = 0.0;
    float m_delta_time = 0.0f;
    double m_last_x = 0.0f;
    double m_last_y = 0.0f;
    float m_angle = 0.0;
};

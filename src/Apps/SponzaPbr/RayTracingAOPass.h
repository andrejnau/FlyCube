#pragma once

#include "GeometryPass.h"
#include "Settings.h"
#include <Context/DX11Context.h>
#include <Geometry/Geometry.h>
#include <d3d11.h>
#include <wrl.h>
#include <ProgramRef/RayTracingAO.h>
#include <ProgramRef/SSAOPassVS.h>
#include <ProgramRef/SSAOBlurPassPS.h>

using namespace Microsoft::WRL;

class RayTracingAOPass : public IPass, public IModifySettings
{
public:
    struct Input
    {
        GeometryPass::Output& geometry_pass;
        SceneModels& scene_list;
        Model& square;
        Camera& camera;
    };

    struct Output
    {
        Resource::Ptr ao;
    } output;

    RayTracingAOPass(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySettings(const Settings & settings) override;

private:
    void CreateSizeDependentResources();

    Settings m_settings;
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    Program<RayTracingAO> m_raytracing_program;
    Program<SSAOBlurPassPS, SSAOPassVS> m_program_blur;

    std::vector<Resource::Ptr> m_bottom;
    Resource::Ptr m_top;
    Resource::Ptr m_indices;
    Resource::Ptr m_positions;
    bool m_is_initialized = false;
    Resource::Ptr m_ao;
    Resource::Ptr m_ao_blur;
};


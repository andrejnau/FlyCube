#pragma once

#include "GeometryPass.h"
#include "SSAOPass.h"
#include "ShadowPass.h"
#include "IrradianceConversion.h"
#include "Settings.h"

#include <Scene/SceneBase.h>
#include <Context/Context.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/LightPassPS.h>
#include <ProgramRef/LightPassVS.h>

class LightPass : public IPass, public IModifySettings
{
public:
    struct Input
    {
        GeometryPass::Output& geometry_pass;
        ShadowPass::Output& shadow_pass;
        SSAOPass::Output& ssao_pass;
        Resource::Ptr ray_tracing_ao;
        Model& model;
        Camera& camera;
        glm::vec3& light_pos;
        Resource::Ptr& irradince;
        Resource::Ptr& prefilter;
        Resource::Ptr& brdf;
    };

    struct Output
    {
        Resource::Ptr rtv;
    } output;

    LightPass(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySettings(const Settings & settings) override;

private:
    void CreateSizeDependentResources();
    void SetDefines(Program<LightPassPS, LightPassVS>& program);

    Settings m_settings;
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    Program<LightPassPS, LightPassVS> m_program;
    Resource::Ptr m_depth_stencil_view;
    Resource::Ptr m_sampler;
    Resource::Ptr m_sampler_brdf;
    Resource::Ptr m_compare_sampler;
};

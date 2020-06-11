#pragma once

#include "GeometryPass.h"
#include "SSAOPass.h"
#include "ShadowPass.h"
#include "IrradianceConversion.h"
#include "SponzaSettings.h"

#include "RenderPass.h"
#include <Context/Context.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/LightPassPS.h>
#include <ProgramRef/LightPassVS.h>

class LightPass : public IPass, public IModifySponzaSettings
{
public:
    struct Input
    {
        GeometryPass::Output& geometry_pass;
        ShadowPass::Output& shadow_pass;
        SSAOPass::Output& ssao_pass;
        std::shared_ptr<Resource>*& ray_tracing_ao;
        Model& model;
        Camera& camera;
        glm::vec3& light_pos;
        std::shared_ptr<Resource>& irradince;
        std::shared_ptr<Resource>& prefilter;
        std::shared_ptr<Resource>& brdf;
    };

    struct Output
    {
        std::shared_ptr<Resource> rtv;
    } output;

    LightPass(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySponzaSettings(const SponzaSettings& settings) override;

private:
    void CreateSizeDependentResources();
    void SetDefines(ProgramHolder<LightPassPS, LightPassVS>& program);

    SponzaSettings m_settings;
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    ProgramHolder<LightPassPS, LightPassVS> m_program;
    std::shared_ptr<Resource> m_depth_stencil_view;
    std::shared_ptr<Resource> m_sampler;
    std::shared_ptr<Resource> m_sampler_brdf;
    std::shared_ptr<Resource> m_compare_sampler;
};

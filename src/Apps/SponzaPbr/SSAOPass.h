#pragma once

#include "GeometryPass.h"
#include "Settings.h"
#include <Context/Context.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/SSAOPassPS.h>
#include <ProgramRef/SSAOPassVS.h>
#include <ProgramRef/SSAOBlurPassPS.h>

class SSAOPass : public IPass, public IModifySettings
{
public:
    struct Input
    {
        GeometryPass::Output& geometry_pass;
        Model& square;
        Camera& camera;
    };

    struct Output
    {
        Resource::Ptr ao;
    } output;

    SSAOPass(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySettings(const Settings & settings) override;

private:
    void SetDefines(Program<SSAOPassPS, SSAOPassVS>& program);
    void CreateSizeDependentResources();

    Settings m_settings;
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    Resource::Ptr m_noise_texture;
    Resource::Ptr m_depth_stencil_view;
    Program<SSAOPassPS, SSAOPassVS> m_program;
    Program<SSAOBlurPassPS, SSAOPassVS> m_program_blur;
    Resource::Ptr m_ao;
    Resource::Ptr m_ao_blur;
};

#pragma once

#include "GeometryPass.h"
#include "Settings.h"
#include <Context/DX11Context.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/SSAOPassPS.h>
#include <ProgramRef/SSAOPassVS.h>
#include <ProgramRef/SSAOBlurPassPS.h>
#include <d3d11.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class SSAOPass : public IPass, public IModifySettings
{
public:
    struct Input
    {
        GeometryPass::Output& geometry_pass;
        Model& model;
        Camera& camera;
    };

    struct Output
    {
        Resource::Ptr srv;
        Resource::Ptr srv_blur;
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
    ComPtr<ID3D11SamplerState> m_texture_sampler;
    Resource::Ptr m_depth_stencil_view;
    Program<SSAOPassPS, SSAOPassVS> m_program;
    Program<SSAOBlurPassPS, SSAOPassVS> m_program_blur;
};

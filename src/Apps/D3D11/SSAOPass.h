#pragma once

#include "GeometryPass.h"
#include <Context/Context.h>
#include <Geometry/DX11Geometry.h>
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
        Model<DX11Mesh>& model;
        Camera& camera;
    };

    struct Output
    {
        ComPtr<ID3D11Resource> srv;
        ComPtr<ID3D11Resource> srv_blur;
    } output;

    SSAOPass(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySettings(const Settings & settings) override;

private:
    void SetDefines(Program<SSAOPassPS, SSAOPassVS>& program);

    Settings m_settings;
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    ComPtr<ID3D11Texture2D> m_noise_texture;
    ComPtr<ID3D11SamplerState> m_texture_sampler;
    ComPtr<ID3D11DepthStencilView> m_depth_stencil_view;
    ComPtr<ID3D11RenderTargetView> m_rtv;
    ComPtr<ID3D11RenderTargetView> m_rtv_blur;
    Program<SSAOPassPS, SSAOPassVS> m_program;
    Program<SSAOBlurPassPS, SSAOPassVS> m_program_blur;
};

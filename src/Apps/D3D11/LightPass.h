#pragma once

#include "D3D11/GeometryPass.h"
#include "D3D11/ShadowPass.h"

#include <Scene/SceneBase.h>
#include <Context/Context.h>
#include <Geometry/DX11Geometry.h>
#include <ProgramRef/LightPassPS.h>
#include <ProgramRef/LightPassVS.h>
#include <d3d11.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class LightPass : public IPass, public IModifySettings
{
public:
    struct Input
    {
        GeometryPass::Output& geometry_pass;
        ShadowPass::Output& shadow_pass;
        Model<DX11Mesh>& model;
        Camera& camera;
        glm::vec3& light_pos;
    };

    struct Output
    {
        ComPtr<ID3D11ShaderResourceView> srv;
    } output;

    void SetDefines(Program<LightPassPS, LightPassVS>& program);

    LightPass(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySettings(const Settings & settings) override;

    void CreateRtvSrv(ComPtr<ID3D11RenderTargetView>& rtv, ComPtr<ID3D11ShaderResourceView>& srv);

    void CreateDsv();

private:
    Settings m_settings;
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    Program<LightPassPS, LightPassVS> m_program;
    ComPtr<ID3D11SamplerState> m_shadow_sampler;
    ComPtr<ID3D11RasterizerState> m_rasterizer_state;
    ComPtr<ID3D11DepthStencilView> m_depth_stencil_view;
    ComPtr<ID3D11Texture2D> m_depth_stencil_buffer;
    ComPtr<ID3D11RenderTargetView> m_rtv;
};

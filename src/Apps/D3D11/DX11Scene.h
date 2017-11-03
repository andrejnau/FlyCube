#pragma once

#include <Scene/SceneBase.h>
#include <Context/Context.h>
#include <Geometry/DX11Geometry.h>
#include <d3d11.h>
#include <DXGI1_4.h>
#include <wrl.h>
#include <string>

#include <Program/Program.h>
#include <ProgramRef/GeometryPassPS.h>
#include <ProgramRef/GeometryPassVS.h>
#include <ProgramRef/LightPassPS.h>
#include <ProgramRef/LightPassVS.h>

#include "D3D11/GeometryPass.h"

using namespace Microsoft::WRL;

class DX11Scene : public SceneBase
{
public:
    DX11Scene(int width, int height);

    static IScene::Ptr Create(int width, int height);

    virtual void OnUpdate() override;
    void LightPass();
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;

private:
    void CreateRT();
    void CreateViewPort();
    void CreateSampler();

    ComPtr<ID3D11RenderTargetView> m_render_target_view;
    ComPtr<ID3D11DepthStencilView> m_depth_stencil_view;
    ComPtr<ID3D11Texture2D> m_depth_stencil_buffer;

    D3D11_VIEWPORT m_viewport;

    ComPtr<ID3D11SamplerState> m_texture_sampler;

    int m_width;
    int m_height;
    Context m_context;
    Program<LightPassPS, LightPassVS> m_shader_light_pass;
    Model<DX11Mesh> m_model_of_file;
    Model<DX11Mesh> m_model_square;
    GeometryPass::Input m_geometry_pass_input;
    GeometryPass m_geometry_pass;
};

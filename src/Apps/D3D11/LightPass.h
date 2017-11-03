#pragma once

#include "D3D11/GeometryPass.h"

#include <Scene/SceneBase.h>
#include <Context/Context.h>
#include <Geometry/DX11Geometry.h>
#include <ProgramRef/LightPassPS.h>
#include <ProgramRef/LightPassVS.h>
#include <d3d11.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class LightPass : public IScene
{
public:
    struct Input
    {
        GeometryPass::Output& geometry_pass;
        Model<DX11Mesh>& model;
        Camera& camera;
        ComPtr<ID3D11RenderTargetView>& render_target_view;
        ComPtr<ID3D11DepthStencilView>& depth_stencil_view;
    };

    struct Output
    {
    } output;

    LightPass(Context& context, Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;

private:
    Context& m_context;
    Input& m_input;
    int m_width;
    int m_height;
    Program<LightPassPS, LightPassVS> m_program;
};

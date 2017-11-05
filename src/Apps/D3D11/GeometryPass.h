#pragma once

#include <Scene/SceneBase.h>
#include <Context/Context.h>
#include <Geometry/DX11Geometry.h>
#include <ProgramRef/GeometryPassPS.h>
#include <ProgramRef/GeometryPassVS.h>
#include <d3d11.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class GeometryPass : public IPass
{
public:
    struct Input
    {
        Model<DX11Mesh>& model;
        Camera& camera;
        ComPtr<ID3D11DepthStencilView>& depth_stencil_view;
    };

    struct Output
    {
        ComPtr<ID3D11ShaderResourceView> position_srv;
        ComPtr<ID3D11ShaderResourceView> normal_srv;
        ComPtr<ID3D11ShaderResourceView> ambient_srv;
        ComPtr<ID3D11ShaderResourceView> diffuse_srv;
        ComPtr<ID3D11ShaderResourceView> specular_srv;
    } output;

    GeometryPass(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;

private:
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    Program<GeometryPassPS, GeometryPassVS> m_program;

    void CreateRtvSrv(ComPtr<ID3D11RenderTargetView>& rtv, ComPtr<ID3D11ShaderResourceView>& srv);
    void InitGBuffers();

    ComPtr<ID3D11RenderTargetView> m_position_rtv;
    ComPtr<ID3D11RenderTargetView> m_normal_rtv;
    ComPtr<ID3D11RenderTargetView> m_ambient_rtv;
    ComPtr<ID3D11RenderTargetView> m_diffuse_rtv;
    ComPtr<ID3D11RenderTargetView> m_specular_rtv;
};


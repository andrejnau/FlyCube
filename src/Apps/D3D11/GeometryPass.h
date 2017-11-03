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

using namespace Microsoft::WRL;


class GeometryPass : public IScene
{
public:
    GeometryPass(Context& context, Model<DX11Mesh>& model, Camera& camera, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;

    void CreaetDepth();

    ComPtr<ID3D11ShaderResourceView> m_position_srv;
    ComPtr<ID3D11ShaderResourceView> m_normal_srv;
    ComPtr<ID3D11ShaderResourceView> m_ambient_srv;
    ComPtr<ID3D11ShaderResourceView> m_diffuse_srv;
    ComPtr<ID3D11ShaderResourceView> m_specular_srv;

private:
    Context& m_context;
    Model<DX11Mesh>& m_model;
    Camera& m_camera;
    int m_width;
    int m_height;
    Program<GeometryPassPS, GeometryPassVS> m_program;

    void CreateRtvSrv(ComPtr<ID3D11RenderTargetView>& rtv, ComPtr<ID3D11ShaderResourceView>& srv);
    void InitGBuffers();

    ComPtr<ID3D11DepthStencilView> m_depth_stencil_view;
    ComPtr<ID3D11Texture2D> m_depth_stencil_buffer;

    ComPtr<ID3D11RenderTargetView> m_position_rtv;
    ComPtr<ID3D11RenderTargetView> m_normal_rtv;
    ComPtr<ID3D11RenderTargetView> m_ambient_rtv;
    ComPtr<ID3D11RenderTargetView> m_diffuse_rtv;
    ComPtr<ID3D11RenderTargetView> m_specular_rtv;
};


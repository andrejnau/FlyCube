#pragma once

#include <Scene/SceneBase.h>
#include <Context/Context.h>
#include <Geometry/DX11Geometry.h>
#include <ProgramRef/GeometryPassPS.h>
#include <ProgramRef/GeometryPassVS.h>
#include <d3d11.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class GeometryPass : public IPass, public IModifySettings
{
public:
    struct Input
    {
        SceneList<DX11Mesh>& scene_list;
        Camera& camera;
    };

    struct Output
    {
        ComPtr<ID3D11ShaderResourceView> position_srv;
        ComPtr<ID3D11ShaderResourceView> normal_srv;
        ComPtr<ID3D11ShaderResourceView> ambient_srv;
        ComPtr<ID3D11ShaderResourceView> diffuse_srv;
        ComPtr<ID3D11ShaderResourceView> specular_srv;
        ComPtr<ID3D11ShaderResourceView> position_view_srv;
        ComPtr<ID3D11ShaderResourceView> normal_view_srv;
    } output;

    GeometryPass(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySettings(const Settings& settings) override;

private:
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    Program<GeometryPassPS, GeometryPassVS> m_program;

    void InitGBuffers();

    ComPtr<ID3D11RenderTargetView> m_position_rtv;
    ComPtr<ID3D11RenderTargetView> m_normal_rtv;
    ComPtr<ID3D11RenderTargetView> m_ambient_rtv;
    ComPtr<ID3D11RenderTargetView> m_diffuse_rtv;
    ComPtr<ID3D11RenderTargetView> m_specular_rtv;
    ComPtr<ID3D11DepthStencilView> m_depth_stencil_view;
    Settings m_settings;
};


#pragma once

#include <Scene/SceneBase.h>
#include <Geometry/DX11Geometry.h>
#include <Program/DX11Program.h>
#include <d3d11.h>
#include <DXGI1_4.h>
#include <wrl.h>
#include <string>

using namespace Microsoft::WRL;

class DXSample : public SceneBase
{
public:
    DXSample();
    ~DXSample();

    static IScene::Ptr Create();

    virtual void OnInit(int width, int height) override;
    virtual void OnUpdate() override;
    void GeometryPass();
    void LightPass();
    virtual void OnRender() override;
    virtual void OnDestroy() override;
    virtual void OnSizeChanged(int width, int height) override;
    virtual glm::mat4 StoreMatrix(const glm::mat4& m) override;

private:
    void CreateDeviceAndSwapChain();
    void CreateRT();
    void CreateViewPort();
    void CreateSampler();

    std::unique_ptr<Model<DX11Mesh>> CreateGeometry(const std::string & path);

    void CreateRTV(ComPtr<ID3D11RenderTargetView>& rtv, ComPtr<ID3D11ShaderResourceView>& srv);

    void InitGBuffer();

    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_device_context;
    ComPtr<IDXGISwapChain> m_swap_chain;

    ComPtr<ID3D11RenderTargetView> m_render_target_view;
    ComPtr<ID3D11DepthStencilView> m_depth_stencil_view;
    ComPtr<ID3D11Texture2D> m_depth_stencil_buffer;

    D3D11_VIEWPORT m_viewport;

    ComPtr<ID3D11SamplerState> m_texture_sampler;

    std::unique_ptr<DXShader> m_shader_geometry_pass;
    std::unique_ptr<DXShader> m_shader_light_pass;

    std::unique_ptr<Model<DX11Mesh>> m_model_of_file;
    std::unique_ptr<Model<DX11Mesh>> m_model_square;

    ComPtr<ID3D11RenderTargetView> m_position_rtv;
    ComPtr<ID3D11RenderTargetView> m_normal_rtv;
    ComPtr<ID3D11RenderTargetView> m_ambient_rtv;
    ComPtr<ID3D11RenderTargetView> m_diffuse_rtv;
    ComPtr<ID3D11RenderTargetView> m_specular_rtv;

    ComPtr<ID3D11ShaderResourceView> m_position_srv;
    ComPtr<ID3D11ShaderResourceView> m_normal_srv;
    ComPtr<ID3D11ShaderResourceView> m_ambient_srv;
    ComPtr<ID3D11ShaderResourceView> m_diffuse_srv;
    ComPtr<ID3D11ShaderResourceView> m_specular_srv;

    int m_width;
    int m_height;
    const bool m_use_rotare = true;
};
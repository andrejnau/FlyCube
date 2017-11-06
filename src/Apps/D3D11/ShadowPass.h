#pragma once

#include <Scene/SceneBase.h>
#include <Context/Context.h>
#include <Geometry/DX11Geometry.h>
#include <ProgramRef/ShadowPassVS.h>
#include <ProgramRef/ShadowPassGS.h>
#include <d3d11.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class ShadowPass : public IPass
{
public:
    struct Input
    {
        Model<DX11Mesh>& model;
        Camera& camera;
        glm::vec3& light_pos;
    };

    struct Output
    {
        ComPtr<ID3D11ShaderResourceView> srv;
        ComPtr<ID3D11ShaderResourceView> cubemap_id;
    } output;

    ShadowPass(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;

    void CreateTexture();

    void CreateTextureId();

    void CreateViewPort();

private:
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    int m_size = 1024;
    ComPtr<ID3D11DepthStencilView> m_depth_stencil_view;
    Program<ShadowPassVS, ShadowPassGS> m_program;
    D3D11_VIEWPORT m_viewport;
};


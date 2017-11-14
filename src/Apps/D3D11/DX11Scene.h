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
#include "D3D11/LightPass.h"
#include "D3D11/ImGuiPass.h"
#include "D3D11/ShadowPass.h"
#include "D3D11/GenerateMipmapPass.h"

using namespace Microsoft::WRL;

class DX11Scene : public SceneBase
{
public:
    DX11Scene(GLFWwindow* window, int width, int height);

    static IScene::Ptr Create(GLFWwindow* window, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;

    virtual void OnKey(int key, int action) override;
    virtual void OnMouse(bool first, double xpos, double ypos) override;
    virtual void OnMouseButton(int button, int action) override;
    virtual void OnScroll(double xoffset, double yoffset) override;
    virtual void OnInputChar(unsigned int ch) override;

private:
    void CreateRT();
    void CreateViewPort();
    void CreateSampler();

    ComPtr<ID3D11RenderTargetView> m_render_target_view;
    ComPtr<ID3D11DepthStencilView> m_depth_stencil_view;
    ComPtr<ID3D11Texture2D> m_depth_stencil_buffer;

    D3D11_VIEWPORT m_viewport;

    ComPtr<ID3D11SamplerState> m_texture_sampler;
    glm::vec3 light_pos;

    int m_width;
    int m_height;
    Context m_context;
    Model<DX11Mesh> m_model_of_file;
    Model<DX11Mesh> m_model_square;
    GeometryPass m_geometry_pass;
    ShadowPass m_shadow_pass;
    GenerateMipmapPass m_generate_mipmap_pass;
    LightPass m_light_pass;
    ImGuiPass m_imgui_pass;
};

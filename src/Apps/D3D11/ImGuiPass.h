#pragma once

#include <Scene/SceneBase.h>
#include <Context/Context.h>
#include <Geometry/DX11Geometry.h>
#include <d3d11.h>
#include <wrl.h>
#include <imgui.h>
#include <ProgramRef/ImGuiPassPS.h>
#include <ProgramRef/ImGuiPassVS.h>

using namespace Microsoft::WRL;

class ImGuiPass : public IPass
                , public IInput
{
public:
    struct Input
    {
        IModifySettings& root_scene;
    };

    struct Output
    {
    } output;

    void RenderDrawLists(ImDrawData * draw_data);

    void CreateFontsTexture();

    bool ImGui_ImplDX11_Init();

    ImGuiPass(Context& context, const Input& input, int width, int height);
    ~ImGuiPass();

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;

    virtual void OnKey(int key, int action) override;
    virtual void OnMouse(bool first, double xpos, double ypos) override;
    virtual void OnMouseButton(int button, int action) override;
    virtual void OnScroll(double xoffset, double yoffset) override;
    virtual void OnInputChar(unsigned int ch) override;

private:
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;

    // Data
    INT64                    m_time = 0;
    INT64                    m_ticks_per_second = 0;

    ComPtr<ID3D11SamplerState> m_font_sampler;
    ComPtr<ID3D11ShaderResourceView> m_font_texture_view;
    ComPtr<ID3D11RasterizerState>   m_rasterizer_state;
    ComPtr<ID3D11BlendState>        m_blend_state;
    ComPtr<ID3D11DepthStencilState> m_depth_stencil_state;

    Program<ImGuiPassPS, ImGuiPassVS> m_program;
};

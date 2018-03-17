#pragma once

#include <Scene/SceneBase.h>
#include <Context/DX11Context.h>
#include <Geometry/Geometry.h>
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
        Resource::Ptr& rtv;
        IModifySettings& root_scene;
    };

    struct Output
    {
    } output;

    void RenderDrawLists(ImDrawData * draw_data);

    void CreateFontsTexture();

    bool InitImGui();

    ImGuiPass(Context& DX11Context, const Input& input, int width, int height);
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
    Context & m_context;
    Input m_input;
    int m_width;
    int m_height;

    // Data
    INT64                    m_time = 0;
    INT64                    m_ticks_per_second = 0;

    Resource::Ptr m_font_texture_view;

    Program<ImGuiPassPS, ImGuiPassVS> m_program;
    PerFrameData<std::unique_ptr<IAVertexBuffer>> m_positions_buffer;
    PerFrameData<std::unique_ptr<IAVertexBuffer>> m_texcoords_buffer;
    PerFrameData<std::unique_ptr<IAVertexBuffer>> m_colors_buffer;
    PerFrameData<std::unique_ptr<IAIndexBuffer>> m_indices_buffer;
};

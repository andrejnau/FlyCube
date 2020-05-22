#pragma once

#include <imgui.h>
#include <Scene/SceneBase.h>
#include <Context/Context.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/ImGuiPassPS.h>
#include <ProgramRef/ImGuiPassVS.h>
#include "ImGuiSettings.h"

class ImGuiPass : public IPass
                , public InputEvents
{
public:
    struct Input
    {
        std::shared_ptr<Resource>& rtv;
        IModifySponzaSettings& root_scene;
    };

    struct Output
    {
    } output;

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
    void CreateFontsTexture();
    void InitKey();

    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;

    std::shared_ptr<Resource> m_font_texture_view;
    ProgramHolder<ImGuiPassPS, ImGuiPassVS> m_program;
    PerFrameData<std::unique_ptr<IAVertexBuffer>> m_positions_buffer;
    PerFrameData<std::unique_ptr<IAVertexBuffer>> m_texcoords_buffer;
    PerFrameData<std::unique_ptr<IAVertexBuffer>> m_colors_buffer;
    PerFrameData<std::unique_ptr<IAIndexBuffer>> m_indices_buffer;
    std::shared_ptr<Resource> m_sampler;
    ImGuiSettings m_settings;
};

#pragma once

#include <imgui.h>
#include "RenderPass.h"
#include <Device/Device.h>
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
        SponzaSettings& settings;
    };

    struct Output
    {
    } output;

    ImGuiPass(RenderDevice& device, RenderCommandList& command_list, const Input& input, int width, int height, GLFWwindow* window);
    ~ImGuiPass();

    virtual void OnUpdate() override;
    virtual void OnRender(RenderCommandList& command_list)override;
    virtual void OnResize(int width, int height) override;

    virtual void OnKey(int key, int action) override;
    virtual void OnMouse(bool first, double xpos, double ypos) override;
    virtual void OnMouseButton(int button, int action) override;
    virtual void OnScroll(double xoffset, double yoffset) override;
    virtual void OnInputChar(unsigned int ch) override;

private:
    void CreateFontsTexture(RenderCommandList& command_list);
    void InitKey();

    RenderDevice& m_device;
    Input m_input;
    int m_width;
    int m_height;
    GLFWwindow* m_window;

    std::shared_ptr<Resource> m_font_texture_view;
    ProgramHolder<ImGuiPassPS, ImGuiPassVS> m_program;
    std::array<std::unique_ptr<IAVertexBuffer>, 3> m_positions_buffer;
    std::array<std::unique_ptr<IAVertexBuffer>, 3> m_texcoords_buffer;
    std::array<std::unique_ptr<IAVertexBuffer>, 3> m_colors_buffer;
    std::array<std::unique_ptr<IAIndexBuffer>, 3> m_indices_buffer;
    std::shared_ptr<Resource> m_sampler;
    ImGuiSettings m_imgui_settings;
};

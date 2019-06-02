#include "ImGuiPass.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Geometry/IABuffer.h>
#include <Program/DX11ProgramApi.h>
#include <Texture/DXGIFormatHelper.h>
#include "ImGuiStateBackup.h"

ImGuiPass::ImGuiPass(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program(context)
    , m_positions_buffer(context)
    , m_texcoords_buffer(context)
    , m_colors_buffer(context)
    , m_indices_buffer(context)
    , m_settings(m_input.root_scene)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);

    InitKey();
    CreateFontsTexture();
    m_sampler = m_context.CreateSampler({
        SamplerFilter::kMinMagMipLinear,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kAlways });
}

ImGuiPass::~ImGuiPass()
{
    ImGui::Shutdown();
}

void ImGuiPass::OnUpdate()
{
    if (glfwGetInputMode(m_context.GetWindow(), GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        return;

    m_settings.NewFrame();

    ImGui::Render();

    ImDrawData* draw_data = ImGui::GetDrawData();

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec4> colors;
    std::vector<uint32_t> indices;

    for (int i = 0; i < draw_data->CmdListsCount; ++i)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[i];
        for (int j = 0; j < cmd_list->VtxBuffer.Size; ++j)
        {
            auto& pos = cmd_list->VtxBuffer.Data[j].pos;
            auto& uv = cmd_list->VtxBuffer.Data[j].uv;
            uint8_t* col = reinterpret_cast<uint8_t*>(&cmd_list->VtxBuffer.Data[j].col);
            positions.push_back({ pos.x, pos.y, 0.0 });
            texcoords.push_back({ uv.x, uv.y });
            colors.push_back({ col[0] / 255.0, col[1] / 255.0, col[2] / 255.0, col[3] / 255.0 });
        }
        for (int j = 0; j < cmd_list->IdxBuffer.Size; ++j)
        {
            indices.push_back(cmd_list->IdxBuffer.Data[j]);
        }
    }

    m_positions_buffer.get().reset(new IAVertexBuffer(m_context, positions));
    m_texcoords_buffer.get().reset(new IAVertexBuffer(m_context, texcoords));
    m_colors_buffer.get().reset(new IAVertexBuffer(m_context, colors));
    m_indices_buffer.get().reset(new IAIndexBuffer(m_context, indices, gli::format::FORMAT_R32_UINT_PACK32));
}

void ImGuiPass::OnRender()
{
    if (glfwGetInputMode(m_context.GetWindow(), GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        return;

    ImDrawData* draw_data = ImGui::GetDrawData();

    // TODO: remove it
    DX11StateBackup guard(dynamic_cast<DX11Context*>(&m_context));

    m_context.UseProgram(m_program);

    m_program.vs.cbuffer.vertexBuffer.ProjectionMatrix = glm::ortho(0.0f, 1.0f * m_width, 1.0f * m_height, 0.0f);

    m_program.ps.om.rtv0.Attach(m_input.rtv);

    m_indices_buffer.get()->Bind();
    m_positions_buffer.get()->BindToSlot(m_program.vs.ia.POSITION);
    m_texcoords_buffer.get()->BindToSlot(m_program.vs.ia.TEXCOORD);
    m_colors_buffer.get()->BindToSlot(m_program.vs.ia.COLOR);

    m_program.ps.sampler.sampler0.Attach(m_sampler);

    m_program.SetBlendState({
        true,
        Blend::kSrcAlpha,
        Blend::kInvSrcAlpha,
        BlendOp::kAdd,
        Blend::kInvSrcAlpha,
        Blend::kZero,
        BlendOp::kAdd });

    m_program.SetRasterizeState({ FillMode::kSolid, CullMode::kNone });

    m_program.SetDepthStencilState({ false });

    m_context.SetViewport(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);

    int vtx_offset = 0;
    int idx_offset = 0;
    for (int i = 0; i < draw_data->CmdListsCount; ++i)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[i];
        for (int j = 0; j < cmd_list->CmdBuffer.Size; ++j)
        {
            const ImDrawCmd& cmd = cmd_list->CmdBuffer[j];
            m_program.ps.srv.texture0.Attach(*(Resource::Ptr*)cmd.TextureId);
            m_context.SetScissorRect((LONG)cmd.ClipRect.x, (LONG)cmd.ClipRect.y, (LONG)cmd.ClipRect.z, (LONG)cmd.ClipRect.w);
            m_context.DrawIndexed(cmd.ElemCount, idx_offset, vtx_offset);
            idx_offset += cmd.ElemCount;
        }
        vtx_offset += cmd_list->VtxBuffer.Size;
    }
}

void ImGuiPass::OnResize(int width, int height)
{
    ImGuiIO& io = ImGui::GetIO();
    m_width = width;
    m_height = height;
    io.DisplaySize = ImVec2((float)m_width, (float)m_height);
}

void ImGuiPass::OnKey(int key, int action)
{
    if (glfwGetInputMode(m_context.GetWindow(), GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (action == GLFW_PRESS)
            io.KeysDown[key] = true;
        if (action == GLFW_RELEASE)
            io.KeysDown[key] = false;

        io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
        io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
        io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
        io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
    }
    m_settings.OnKey(key, action);
}

void ImGuiPass::OnMouse(bool first, double xpos, double ypos)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(xpos, ypos);
}

void ImGuiPass::OnMouseButton(int button, int action)
{
    ImGuiIO& io = ImGui::GetIO();
    if (button >= 0 && button < 3)
        io.MouseDown[button] = action == GLFW_PRESS;
}

void ImGuiPass::OnScroll(double xoffset, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel += yoffset;
}

void ImGuiPass::OnInputChar(unsigned int ch)
{
    ImGuiIO& io = ImGui::GetIO();
    if (ch > 0 && ch < 0x10000)
        io.AddInputCharacter((unsigned short)ch);
}

void ImGuiPass::CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    m_font_texture_view = m_context.CreateTexture(BindFlag::kSrv, gli::format::FORMAT_RGBA8_UNORM_PACK8, 1, width, height);
    size_t num_bytes = 0;
    size_t row_bytes = 0;
    GetSurfaceInfo(width, height, gli::format::FORMAT_RGBA8_UNORM_PACK8, &num_bytes, &row_bytes, nullptr);
    m_context.UpdateSubresource(m_font_texture_view, 0, pixels, row_bytes, num_bytes);

    // Store our identifier
    io.Fonts->TexID = (void*)&m_font_texture_view;
}

void ImGuiPass::InitKey()
{
    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
}

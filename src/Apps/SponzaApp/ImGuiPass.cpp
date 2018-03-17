#include "ImGuiPass.h"
#include <Geometry/IABuffer.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <Program/DX11ProgramApi.h>
#include "Texture/DXGIFormatHelper.h"

struct DX11StateBackup
{
    struct BACKUP_DX11_STATE
    {
        UINT                        ScissorRectsCount, ViewportsCount;
        D3D11_RECT                  ScissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        D3D11_VIEWPORT              Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        ID3D11RasterizerState*      RS;
        ID3D11BlendState*           BlendState;
        FLOAT                       BlendFactor[4];
        UINT                        SampleMask;
        UINT                        StencilRef;
        ID3D11DepthStencilState*    DepthStencilState;
        ID3D11ShaderResourceView*   PSShaderResource;
        ID3D11SamplerState*         PSSampler;
        ID3D11PixelShader*          PS;
        ID3D11VertexShader*         VS;
        UINT                        PSInstancesCount, VSInstancesCount;
        ID3D11ClassInstance*        PSInstances[256], *VSInstances[256];   // 256 is max according to PSSetShader documentation
        D3D11_PRIMITIVE_TOPOLOGY    PrimitiveTopology;
        ID3D11Buffer*               IndexBuffer, *VertexBuffer, *VSConstantBuffer;
        UINT                        IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
        DXGI_FORMAT                 IndexBufferFormat;
        ID3D11InputLayout*          InputLayout;
    } old;

    ID3D11DeviceContext* ctx = nullptr;;
    DX11StateBackup(DX11Context* context)
    {
        if (!context)
            return;
        ctx = context->device_context.Get();
        // Backup DX state that will be modified to restore it afterwards (unfortunately this is very ugly looking and verbose. Close your eyes!)
        old.ScissorRectsCount = old.ViewportsCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        ctx->RSGetScissorRects(&old.ScissorRectsCount, old.ScissorRects);
        ctx->RSGetViewports(&old.ViewportsCount, old.Viewports);
        ctx->RSGetState(&old.RS);
        ctx->OMGetBlendState(&old.BlendState, old.BlendFactor, &old.SampleMask);
        ctx->OMGetDepthStencilState(&old.DepthStencilState, &old.StencilRef);
        ctx->PSGetShaderResources(0, 1, &old.PSShaderResource);
        ctx->PSGetSamplers(0, 1, &old.PSSampler);
        old.PSInstancesCount = old.VSInstancesCount = 256;
        ctx->PSGetShader(&old.PS, old.PSInstances, &old.PSInstancesCount);
        ctx->VSGetShader(&old.VS, old.VSInstances, &old.VSInstancesCount);
        ctx->VSGetConstantBuffers(0, 1, &old.VSConstantBuffer);
        ctx->IAGetPrimitiveTopology(&old.PrimitiveTopology);
        ctx->IAGetIndexBuffer(&old.IndexBuffer, &old.IndexBufferFormat, &old.IndexBufferOffset);
        ctx->IAGetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset);
        ctx->IAGetInputLayout(&old.InputLayout);
    }

    ~DX11StateBackup()
    {
        if (!ctx)
            return;
        // Restore modified DX state
        ctx->RSSetScissorRects(old.ScissorRectsCount, old.ScissorRects);
        ctx->RSSetViewports(old.ViewportsCount, old.Viewports);
        ctx->RSSetState(old.RS); if (old.RS) old.RS->Release();
        ctx->OMSetBlendState(old.BlendState, old.BlendFactor, old.SampleMask); if (old.BlendState) old.BlendState->Release();
        ctx->OMSetDepthStencilState(old.DepthStencilState, old.StencilRef); if (old.DepthStencilState) old.DepthStencilState->Release();
        ctx->PSSetShaderResources(0, 1, &old.PSShaderResource); if (old.PSShaderResource) old.PSShaderResource->Release();
        ctx->PSSetSamplers(0, 1, &old.PSSampler); if (old.PSSampler) old.PSSampler->Release();
        ctx->PSSetShader(old.PS, old.PSInstances, old.PSInstancesCount); if (old.PS) old.PS->Release();
        for (UINT i = 0; i < old.PSInstancesCount; i++) if (old.PSInstances[i]) old.PSInstances[i]->Release();
        ctx->VSSetShader(old.VS, old.VSInstances, old.VSInstancesCount); if (old.VS) old.VS->Release();
        ctx->VSSetConstantBuffers(0, 1, &old.VSConstantBuffer); if (old.VSConstantBuffer) old.VSConstantBuffer->Release();
        for (UINT i = 0; i < old.VSInstancesCount; i++) if (old.VSInstances[i]) old.VSInstances[i]->Release();
        ctx->IASetPrimitiveTopology(old.PrimitiveTopology);
        ctx->IASetIndexBuffer(old.IndexBuffer, old.IndexBufferFormat, old.IndexBufferOffset); if (old.IndexBuffer) old.IndexBuffer->Release();
        ctx->IASetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset); if (old.VertexBuffer) old.VertexBuffer->Release();
        ctx->IASetInputLayout(old.InputLayout); if (old.InputLayout) old.InputLayout->Release();
    }
};

void ImGuiPass::RenderDrawLists(ImDrawData* draw_data)
{
    DX11StateBackup guard(dynamic_cast<DX11Context*>(&m_context));

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec4> colors;
    std::vector<uint32_t> indices;

    for (int n = 0; n < draw_data->CmdListsCount; ++n)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int j = 0; j < cmd_list->VtxBuffer.Size; ++j)
        {
            auto& pos = cmd_list->VtxBuffer.Data[j].pos;
            auto& uv = cmd_list->VtxBuffer.Data[j].uv;
            uint8_t* col = reinterpret_cast<uint8_t*>(&cmd_list->VtxBuffer.Data[j].col);
            positions.push_back({ pos.x, pos.y, 0.0 });
            texcoords.push_back({ uv.x, uv.y });
            colors.push_back({ *(col + 0) / 255.0, *(col + 1) / 255.0,*(col + 2) / 255.0,*(col + 3)/255.0 });
        }
        for (int j = 0; j < cmd_list->IdxBuffer.Size; ++j)
        {
            indices.push_back(cmd_list->IdxBuffer.Data[j]);
        }
    }

    m_positions_buffer[m_context.GetFrameIndex()].reset(new IAVertexBuffer(m_context, positions));
    m_texcoords_buffer[m_context.GetFrameIndex()].reset(new IAVertexBuffer(m_context, texcoords));
    m_colors_buffer[m_context.GetFrameIndex()].reset(new IAVertexBuffer(m_context, colors));
    m_indices_buffer[m_context.GetFrameIndex()].reset(new IAIndexBuffer(m_context, indices, DXGI_FORMAT_R32_UINT));

    m_program.vs.cbuffer.vertexBuffer.ProjectionMatrix = glm::ortho(0.0f, 1.0f * m_width, 1.0f* m_height, 0.0f);

    m_program.UseProgram();

    m_program.ps.om.rtv0.Attach(m_input.rtv);

    m_indices_buffer[m_context.GetFrameIndex()]->Bind();
    m_positions_buffer[m_context.GetFrameIndex()]->BindToSlot(m_program.vs.ia.POSITION);
    m_texcoords_buffer[m_context.GetFrameIndex()]->BindToSlot(m_program.vs.ia.TEXCOORD);
    m_colors_buffer[m_context.GetFrameIndex()]->BindToSlot(m_program.vs.ia.COLOR);

    m_program.ps.sampler.sampler0.Attach({
        SamplerFilter::kMinMagMipLinear,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kAlways });

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
    for (int n = 0; n < draw_data->CmdListsCount; ++n)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; ++cmd_i)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            m_program.ps.srv.texture0.Attach(*(Resource::Ptr*)pcmd->TextureId);
            m_context.SetScissorRect((LONG)pcmd->ClipRect.x, (LONG)pcmd->ClipRect.y, (LONG)pcmd->ClipRect.z, (LONG)pcmd->ClipRect.w);
            m_context.DrawIndexed(pcmd->ElemCount, idx_offset, vtx_offset);
            idx_offset += pcmd->ElemCount;  
        }
        vtx_offset += cmd_list->VtxBuffer.Size;
    }
}

void ImGuiPass::CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    m_font_texture_view = m_context.CreateTexture(BindFlag::kSrv, DXGI_FORMAT_R8G8B8A8_UNORM, 1, width, height);
    size_t num_bytes = 0;
    size_t row_bytes = 0;
    GetSurfaceInfo(width, height, DXGI_FORMAT_R8G8B8A8_UNORM, &num_bytes, &row_bytes, nullptr);
    m_context.UpdateSubresource(m_font_texture_view, 0, pixels, row_bytes, num_bytes);

    // Store our identifier
    io.Fonts->TexID = (void *)&m_font_texture_view;
}

bool ImGuiPass::InitImGui()
{
    if (!QueryPerformanceFrequency((LARGE_INTEGER *)&m_ticks_per_second))
        return false;
    if (!QueryPerformanceCounter((LARGE_INTEGER *)&m_time))
        return false;

    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
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
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    return true;
}

ImGuiPass::ImGuiPass(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program(context)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);

    InitImGui();
    CreateFontsTexture();
}

ImGuiPass::~ImGuiPass()
{
    ImGui::Shutdown();
}

class ImGuiSettings
{
public:
    ImGuiSettings()
    {
        for (uint32_t i = 2; i <= 8; i *= 2)
        {
            msaa_str.push_back("x" + std::to_string(i));
            msaa.push_back(i);
            if (i == settings.msaa_count)
                msaa_idx = msaa.size() - 1;
        }
    }

    void NewFrame(IModifySettings& listener)
    {
        bool modify_settings = false;
        ImGui::NewFrame();

        ImGui::Begin("Settings");

        static auto fn = [](void* data, int idx, const char** out_text) -> bool
        {
            if (!data || !out_text)
                return false;
            const auto& self = *(ImGuiSettings*)data;
            *out_text = self.msaa_str[idx].c_str();
            return true;
        };

        if (ImGui::Combo("MSAA", &msaa_idx, fn, this, msaa_str.size()))
        {
            settings.msaa_count = msaa[msaa_idx];
            modify_settings = true;
        }

        if (ImGui::SliderFloat("s_near", &settings.s_near, 0.1f, 8.0f, "%.3f"))
        {
            modify_settings = true;
        }

        if (ImGui::SliderFloat("s_far", &settings.s_far, 0.1f, 1024.0f, "%.3f"))
        {
            modify_settings = true;
        }

        if (ImGui::SliderInt("s_size", &settings.s_size, 512, 4096))
        {
            modify_settings = true;
        }

        if (ImGui::Checkbox("use_shadow", &settings.use_shadow))
        {
            modify_settings = true;
        }

        if (ImGui::Checkbox("Tone mapping", &settings.use_tone_mapping))
        {
            modify_settings = true;
        }

        if (ImGui::Checkbox("use_occlusion", &settings.use_occlusion))
        {
            modify_settings = true;
        }

        if (ImGui::Checkbox("use_blinn", &settings.use_blinn))
        {
            modify_settings = true;
        }

        if (ImGui::SliderInt("ssao_scale", &settings.ssao_scale, 1, 8))
        {
            modify_settings = true;
        }

        if (ImGui::SliderFloat("Exposure", &settings.Exposure, 0, 2))
        {
            modify_settings = true;
        }

        if (ImGui::SliderFloat("White", &settings.White, 0, 5))
        {
            modify_settings = true;
        }

        if (ImGui::SliderFloat("light_ambient", &settings.light_ambient, 0, 2))
        {
            modify_settings = true;
        }

        if (ImGui::SliderFloat("light_diffuse", &settings.light_diffuse, 0, 2))
        {
            modify_settings = true;
        }

        if (ImGui::SliderFloat("light_specular", &settings.light_specular, 0, 2))
        {
            modify_settings = true;
        }

        ImGui::End();

        if (modify_settings)
            listener.OnModifySettings(settings);
    }

private:
    Settings settings;
    int msaa_idx = 0;
    std::vector<uint32_t> msaa = { 1 };
    std::vector<std::string> msaa_str = { "Off" };
};

void ImGuiPass::OnUpdate()
{
    size_t max_cnt = 2048;
    m_program.SetMaxEvents(max_cnt);

    if (glfwGetInputMode(m_context.GetWindow(), GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        return;

    static ImGuiSettings settings;
    settings.NewFrame(m_input.root_scene);

    ImGui::Render();

    int cnt = 0;
    auto draw_data = ImGui::GetDrawData();
    for (int n = 0; n < draw_data->CmdListsCount; ++n)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int j = 0; j < cmd_list->VtxBuffer.Size; ++j)
        {
            ++cnt;
        }
    }
    assert(cnt <= max_cnt);
}

void ImGuiPass::OnRender()
{
    if (glfwGetInputMode(m_context.GetWindow(), GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
    {
        RenderDrawLists(ImGui::GetDrawData());
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

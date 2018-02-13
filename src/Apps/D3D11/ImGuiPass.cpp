#include "ImGuiPass.h"
#include <Geometry/Geometry.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

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

    ID3D11DeviceContext* ctx;
    DX11StateBackup(ID3D11DeviceContext* ctx)
        : ctx(ctx)
    {
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
    DX11StateBackup guard(m_context.device_context.Get());
    IMesh mesh = {};
    for (int n = 0; n < draw_data->CmdListsCount; ++n)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int j = 0; j < cmd_list->VtxBuffer.Size; ++j)
        {
            auto& pos = cmd_list->VtxBuffer.Data[j].pos;
            auto& uv = cmd_list->VtxBuffer.Data[j].uv;
            uint8_t* col = reinterpret_cast<uint8_t*>(&cmd_list->VtxBuffer.Data[j].col);
            mesh.positions.push_back({ pos.x, pos.y, 0.0 });
            mesh.texcoords.push_back({ uv.x, uv.y });
            mesh.colors.push_back({ *(col + 0) / 255.0, *(col + 1) / 255.0,*(col + 2) / 255.0,*(col + 3)/255.0 });
        }
        for (int j = 0; j < cmd_list->IdxBuffer.Size; ++j)
        {
            mesh.indices.push_back(cmd_list->IdxBuffer.Data[j]);
        }
    }

    DX11Mesh dx11_mesh(m_context, mesh);

    m_program.vs.cbuffer.vertexBuffer.ProjectionMatrix = glm::ortho(0.0f, 1.0f * m_width, 1.0f* m_height, 0.0f);

    m_program.UseProgram(m_context.device_context);
    dx11_mesh.indices_buffer.Bind();
    dx11_mesh.positions_buffer.BindToSlot(m_program.vs.geometry.POSITION);
    dx11_mesh.texcoords_buffer.BindToSlot(m_program.vs.geometry.TEXCOORD);
    dx11_mesh.colors_buffer.BindToSlot(m_program.vs.geometry.COLOR);
    m_context.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context.device_context->IASetInputLayout(m_program.vs.input_layout.Get());

    m_context.device_context->PSSetSamplers(0, 1, m_font_sampler.GetAddressOf());

    const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
    m_context.device_context->OMSetBlendState(m_blend_state.Get(), blend_factor, 0xffffffff);
    m_context.device_context->OMSetDepthStencilState(m_depth_stencil_state.Get(), 0);
    m_context.device_context->RSSetState(m_rasterizer_state.Get());

    D3D11_VIEWPORT vp = {};
    vp.Width = ImGui::GetIO().DisplaySize.x;
    vp.Height = ImGui::GetIO().DisplaySize.y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = vp.TopLeftY = 0.0f;
    m_context.device_context->RSSetViewports(1, &vp);

    int vtx_offset = 0;
    int idx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; ++n)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; ++cmd_i)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            const D3D11_RECT r = { (LONG)pcmd->ClipRect.x, (LONG)pcmd->ClipRect.y, (LONG)pcmd->ClipRect.z, (LONG)pcmd->ClipRect.w };
            m_context.device_context->PSSetShaderResources(0, 1, (ID3D11ShaderResourceView**)&pcmd->TextureId);
            m_context.device_context->RSSetScissorRects(1, &r);
            m_context.device_context->DrawIndexed(pcmd->ElemCount, idx_offset, vtx_offset);
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

    // Upload texture to graphics system
    {
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        ID3D11Texture2D *pTexture = NULL;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = pixels;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        m_context.device->CreateTexture2D(&desc, &subResource, &pTexture);

        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_context.device->CreateShaderResourceView(pTexture, &srvDesc, &m_font_texture_view);
        pTexture->Release();
    }

    // Store our identifier
    io.Fonts->TexID = (void *)m_font_texture_view.Get();

    // Create texture sampler
    {
        D3D11_SAMPLER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.MipLODBias = 0.f;
        desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        desc.MinLOD = 0.f;
        desc.MaxLOD = 0.f;
        m_context.device->CreateSamplerState(&desc, &m_font_sampler);
    }
}

bool  ImGuiPass::ImGui_ImplDX11_Init()
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
    , m_program(context.device)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);

    ImGui_ImplDX11_Init();

    // Create the blending setup
    {
        D3D11_BLEND_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.AlphaToCoverageEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        m_context.device->CreateBlendState(&desc, &m_blend_state);
    }

    // Create the rasterizer state
    {
        D3D11_RASTERIZER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.ScissorEnable = true;
        desc.DepthClipEnable = true;
        m_context.device->CreateRasterizerState(&desc, &m_rasterizer_state);
    }

    // Create depth-stencil State
    {
        D3D11_DEPTH_STENCIL_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.DepthEnable = false;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        desc.StencilEnable = false;
        desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        desc.BackFace = desc.FrontFace;
        m_context.device->CreateDepthStencilState(&desc, &m_depth_stencil_state);
    }

    CreateFontsTexture();
}

ImGuiPass::~ImGuiPass()
{
    ImGui::Shutdown();
}

class ImGuiSettings
{
public:
    ImGuiSettings(const ComPtr<ID3D11Device>& device)
    {
        for (uint32_t i = 2; i <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; ++i)
        {
            uint32_t quality = 0;
            device->CheckMultisampleQualityLevels(DXGI_FORMAT_R32G32B32A32_FLOAT, i, &quality);
            if (quality == 0)
                continue;
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

        if (ImGui::Checkbox("Tone mapping", &settings.use_tone_mapping))
        {
            modify_settings = true;
        }

        if (ImGui::SliderFloat("Exposure", &settings.Exposure, 0, 2))
        {
            modify_settings = true;
        }

        if (ImGui::SliderFloat("White", &settings.White, 0, 2))
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
    if (glfwGetInputMode(m_context.window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        return;

    static ImGuiSettings settings(m_context.device);
    settings.NewFrame(m_input.root_scene);

    ImGui::Render();
}

void ImGuiPass::OnRender()
{
    if (glfwGetInputMode(m_context.window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
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

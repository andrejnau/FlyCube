#include "ComputeLuminance.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <Utilities/State.h>

ComputeLuminance::ComputeLuminance(DX11Context& DX11Context, const Input& input, int width, int height)
    : m_context(DX11Context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_HDRLum1DPassCS(DX11Context)
    , m_HDRLum2DPassCS(DX11Context)
    , m_HDRApply(DX11Context)
{
}

void ComputeLuminance::OnUpdate()
{
}

ComPtr<IUnknown> ComputeLuminance::GetLum2DPassCS(uint32_t thread_group_x, uint32_t thread_group_y)
{
    m_HDRLum2DPassCS.cs.cbuffer.cbv.dispatchSize = glm::uvec2(thread_group_x, thread_group_y);
    m_HDRLum2DPassCS.UseProgram();

    uint32_t total_invoke = thread_group_x * thread_group_y;

    ComPtr<ID3D11Buffer> buffer;
    D3D11_BUFFER_DESC buffer_desc = {};
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.ByteWidth = sizeof(float) * total_invoke;
    buffer_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    buffer_desc.StructureByteStride = 4;
    ASSERT_SUCCEEDED(m_context.device->CreateBuffer(&buffer_desc, nullptr, &buffer));

    m_HDRLum2DPassCS.cs.uav.result.Attach(buffer);
    m_HDRLum2DPassCS.cs.srv.input.Attach(m_input.hdr_res);
    m_context.device_context->Dispatch(thread_group_x, thread_group_y, 1);

    return buffer;
}

ComPtr<IUnknown> ComputeLuminance::GetLum1DPassCS(ComPtr<IUnknown> input, uint32_t input_buffer_size, uint32_t thread_group_x)
{
    m_HDRLum1DPassCS.cs.cbuffer.cbv.bufferSize = input_buffer_size;
    m_HDRLum1DPassCS.UseProgram();

    ComPtr<ID3D11Buffer> buffer;
    D3D11_BUFFER_DESC buffer_desc = {};
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.ByteWidth = sizeof(float) * thread_group_x;
    buffer_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    buffer_desc.StructureByteStride = 4;
    ASSERT_SUCCEEDED(m_context.device->CreateBuffer(&buffer_desc, nullptr, &buffer));

 
    m_HDRLum1DPassCS.cs.uav.result.Attach(buffer);
    m_HDRLum1DPassCS.cs.srv.input.Attach(input);

    m_context.device_context->Dispatch(thread_group_x, 1, 1);

    m_HDRLum1DPassCS.cs.uav.result.Attach();

    return buffer;
}

void ComputeLuminance::Draw(ComPtr<IUnknown> input)
{
    m_HDRApply.ps.cbuffer.cbv.dim = glm::uvec2(m_width, m_height);
    m_HDRApply.ps.cbuffer.$Globals.Exposure = m_settings.Exposure;
    m_HDRApply.ps.cbuffer.$Globals.White = m_settings.White;
    m_HDRApply.ps.cbuffer.cbv.use_tone_mapping = m_settings.use_tone_mapping;
    m_HDRApply.UseProgram();

    float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_HDRApply.ps.om.rtv0.Attach(m_input.rtv).ClearRenderTarget(color);
    m_HDRApply.ps.om.dsv.Attach(m_input.dsv).ClearDepthStencil(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    m_HDRApply.ps.om.Apply();

    for (DX11Mesh& cur_mesh : m_input.model.meshes)
    {
        cur_mesh.indices_buffer.Bind();
        cur_mesh.positions_buffer.BindToSlot(m_HDRApply.vs.ia.POSITION);
        cur_mesh.texcoords_buffer.BindToSlot(m_HDRApply.vs.ia.TEXCOORD);

        m_HDRApply.ps.srv.hdr_input.Attach(m_input.hdr_res);
        m_HDRApply.ps.srv.lum.Attach(input);
        m_context.DrawIndexed(cur_mesh.indices.size());
    }

    std::vector<ID3D11ShaderResourceView*> empty(D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
    m_context.device_context->PSSetShaderResources(0, empty.size(), empty.data());
}

void ComputeLuminance::OnRender()
{
    m_context.SetViewport(m_width, m_height);

    ComPtr<ID3D11Texture2D> texture;
    m_input.hdr_res.As(&texture);

    D3D11_TEXTURE2D_DESC tex_desc = {};
    texture->GetDesc(&tex_desc);

    uint32_t thread_group_x = (tex_desc.Width + 31) / 32;
    uint32_t thread_group_y = (tex_desc.Height + 31) / 32;

    auto buf = GetLum2DPassCS(thread_group_x, thread_group_y);
    for (int block_size = thread_group_x * thread_group_y; block_size > 1;)
    {
        uint32_t next_block_size = (block_size + 127) / 128;
        buf = GetLum1DPassCS(buf, block_size, next_block_size);
        block_size = next_block_size;
    }

    Draw(buf);
}

void ComputeLuminance::OnResize(int width, int height)
{
}

void ComputeLuminance::OnModifySettings(const Settings& settings)
{
    m_settings = settings;
}

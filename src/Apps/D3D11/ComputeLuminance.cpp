#include "ComputeLuminance.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <Utilities/State.h>

ComputeLuminance::ComputeLuminance(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_HDRLum1DPassCS(context)
    , m_HDRLum2DPassCS(context)
    , m_HDRApply(context)
{
}

void ComputeLuminance::OnUpdate()
{
}

ComPtr<ID3D11ShaderResourceView> ComputeLuminance::GetLum2DPassCS(uint32_t thread_group_x, uint32_t thread_group_y)
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

    ComPtr<ID3D11ShaderResourceView> srv;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.Buffer.FirstElement = 0;
    srv_desc.Buffer.NumElements = total_invoke;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;

    ASSERT_SUCCEEDED(m_context.device->CreateShaderResourceView(buffer.Get(), &srv_desc, &srv));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.Format = DXGI_FORMAT_UNKNOWN;
    uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.NumElements = total_invoke;

    ComPtr<ID3D11UnorderedAccessView> uav;
    m_context.device->CreateUnorderedAccessView(buffer.Get(), &uav_desc, &uav);
    m_context.device_context->CSSetUnorderedAccessViews(m_HDRLum2DPassCS.cs.uav.result, 1, uav.GetAddressOf(), nullptr);
    m_context.device_context->CSSetShaderResources(m_HDRLum2DPassCS.cs.texture.input, 1, m_input.srv.GetAddressOf());
    m_context.device_context->Dispatch(thread_group_x, thread_group_y, 1);

    return srv;
}

ComPtr<ID3D11ShaderResourceView> ComputeLuminance::GetLum1DPassCS(ComPtr<ID3D11ShaderResourceView> input, uint32_t input_buffer_size, uint32_t thread_group_x)
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

    ComPtr<ID3D11ShaderResourceView> srv;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.Buffer.FirstElement = 0;
    srv_desc.Buffer.NumElements = thread_group_x;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;

    ASSERT_SUCCEEDED(m_context.device->CreateShaderResourceView(buffer.Get(), &srv_desc, &srv));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.Format = DXGI_FORMAT_UNKNOWN;
    uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.NumElements = thread_group_x;

    ComPtr<ID3D11UnorderedAccessView> uav;
    m_context.device->CreateUnorderedAccessView(buffer.Get(), &uav_desc, &uav);
    m_context.device_context->CSSetUnorderedAccessViews(m_HDRLum1DPassCS.cs.uav.result, 1, uav.GetAddressOf(), nullptr);

    m_context.device_context->CSSetShaderResources(m_HDRLum1DPassCS.cs.texture.input, 1, input.GetAddressOf());

    m_context.device_context->Dispatch(thread_group_x, 1, 1);

    ID3D11UnorderedAccessView* empty = nullptr;
    m_context.device_context->CSSetUnorderedAccessViews(m_HDRLum1DPassCS.cs.uav.result, 1, &empty, nullptr);

    return srv;
}

void ComputeLuminance::Draw(ComPtr<ID3D11ShaderResourceView> input)
{
    m_HDRApply.ps.cbuffer.cbv.dim = glm::uvec2(m_width, m_height);
    m_HDRApply.ps.cbuffer.$Globals.Exposure = m_settings.Exposure;
    m_HDRApply.ps.cbuffer.$Globals.White = m_settings.White;
    m_HDRApply.ps.cbuffer.cbv.use_tone_mapping = m_settings.use_tone_mapping;
    m_HDRApply.UseProgram();
    m_context.device_context->IASetInputLayout(m_HDRApply.vs.input_layout.Get());

    float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_context.device_context->ClearRenderTargetView(m_input.rtv.Get(), color);
    m_context.device_context->ClearDepthStencilView(m_input.dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_context.device_context->OMSetRenderTargets(1, m_input.rtv.GetAddressOf(), m_input.dsv.Get());

    for (DX11Mesh& cur_mesh : m_input.model.meshes)
    {
        cur_mesh.indices_buffer.Bind();
        cur_mesh.positions_buffer.BindToSlot(m_HDRApply.vs.geometry.POSITION);
        cur_mesh.texcoords_buffer.BindToSlot(m_HDRApply.vs.geometry.TEXCOORD);

        m_context.device_context->PSSetShaderResources(m_HDRApply.ps.texture.hdr_input, 1, m_input.srv.GetAddressOf());
        m_context.device_context->PSSetShaderResources(m_HDRApply.ps.texture.lum, 1, input.GetAddressOf());
        m_context.device_context->DrawIndexed(cur_mesh.indices.size(), 0, 0);
    }

    std::vector<ID3D11ShaderResourceView*> empty(m_HDRApply.ps.texture.hdr_input);
    m_context.device_context->PSSetShaderResources(0, empty.size(), empty.data());
}

void ComputeLuminance::OnRender()
{
    ComPtr<ID3D11Resource> res;
    m_input.srv->GetResource(&res);
    ComPtr<ID3D11Texture2D> texture;
    res.As(&texture);

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

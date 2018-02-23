#include "SSAOPass.h"
#include "DX11CreateUtils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <Utilities/State.h>
#include <chrono>
#include <random>
#include "Texture/DXGIFormatHelper.h"

inline float lerp(float a, float b, float f)
{
    return a + f * (b - a);
}

SSAOPass::SSAOPass(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program(context, std::bind(&SSAOPass::SetDefines, this, std::placeholders::_1))
    , m_program_blur(context)
{
    CreateDsv(m_context, 1, m_width, m_height, m_depth_stencil_view);
    output.srv = CreateRtvSrv(m_context, 1, m_width, m_height, m_rtv);
    output.srv_blur = CreateRtvSrv(m_context, 1, m_width, m_height, m_rtv_blur);

    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    int kernel_size = m_program.ps.cbuffer.SSAOBuffer.samples.size();
    for (int i = 0; i < kernel_size; ++i)
    {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / float(kernel_size);

        // Scale samples s.t. they're more aligned to center of kernel
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        m_program.ps.cbuffer.SSAOBuffer.samples[i] = glm::vec4(sample, 1.0f);
    }

    std::vector<glm::vec3> ssaoNoise;
    for (uint32_t i = 0; i < 16; i++)
    {
        glm::vec3 noise(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, 0.0f);
        ssaoNoise.push_back(noise);
    }

    D3D11_TEXTURE2D_DESC texture_desc = {};
    texture_desc.Width = 4;
    texture_desc.Height = 4;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;

    size_t num_bytes;
    size_t row_bytes;
    GetSurfaceInfo(4, 4, texture_desc.Format, &num_bytes, &row_bytes, nullptr);
    D3D11_SUBRESOURCE_DATA textureBufferData = {};
    textureBufferData.pSysMem = ssaoNoise.data();
    textureBufferData.SysMemPitch = row_bytes;
    textureBufferData.SysMemSlicePitch = num_bytes;

    ComPtr<ID3D11Texture2D> noise_texture_buffer;
    ASSERT_SUCCEEDED(context.device->CreateTexture2D(&texture_desc, &textureBufferData, &m_noise_texture));

    D3D11_SAMPLER_DESC samp_desc = {};
    samp_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samp_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samp_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samp_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samp_desc.MinLOD = 0;
    samp_desc.MaxLOD = D3D11_FLOAT32_MAX;

    ASSERT_SUCCEEDED(m_context.device->CreateSamplerState(&samp_desc, &m_texture_sampler));
}

void SSAOPass::OnUpdate()
{
    m_program.ps.cbuffer.SSAOBuffer.width = m_width;
    m_program.ps.cbuffer.SSAOBuffer.height = m_height;

    glm::mat4 projection, view, model;
    m_input.camera.GetMatrix(projection, view, model);
    m_program.ps.cbuffer.SSAOBuffer.projection = glm::transpose(projection);
    m_program.ps.cbuffer.SSAOBuffer.view = glm::transpose(view);
    m_program.ps.cbuffer.SSAOBuffer.viewInverse = glm::transpose(glm::transpose(glm::inverse(m_input.camera.GetViewMatrix())));
    m_program.ps.cbuffer.SSAOBuffer.ssao_scale = m_settings.ssao_scale;
}

void SSAOPass::OnRender()
{
    if (!m_settings.use_occlusion)
        return;

    m_program.UseProgram();
    m_context.device_context->IASetInputLayout(m_program.vs.input_layout.Get());

    float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_context.device_context->ClearRenderTargetView(m_rtv.Get(), color);

    m_context.device_context->ClearDepthStencilView(m_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    m_context.device_context->OMSetRenderTargets(1, m_rtv.GetAddressOf(), m_depth_stencil_view.Get());

    for (DX11Mesh& cur_mesh : m_input.model.meshes)
    {
        cur_mesh.indices_buffer.Bind();
        cur_mesh.positions_buffer.BindToSlot(m_program.vs.geometry.POSITION);
        cur_mesh.texcoords_buffer.BindToSlot(m_program.vs.geometry.TEXCOORD);

        m_program.ps.srv.gPosition.Attach(m_input.geometry_pass.position);
        m_program.ps.srv.gNormal.Attach(m_input.geometry_pass.normal);

        m_program.ps.srv.noiseTexture.Attach(m_noise_texture);

        m_context.device_context->DrawIndexed(cur_mesh.indices.size(), 0, 0);
    }

    m_context.device_context->OMSetRenderTargets(0, nullptr, nullptr);

    std::vector<ID3D11ShaderResourceView*> empty(D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
    m_context.device_context->PSSetShaderResources(0, empty.size(), empty.data());

    m_program_blur.UseProgram();
    m_context.device_context->IASetInputLayout(m_program_blur.vs.input_layout.Get());

    m_context.device_context->ClearRenderTargetView(m_rtv_blur.Get(), color);
    m_context.device_context->ClearDepthStencilView(m_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    m_context.device_context->OMSetRenderTargets(1, m_rtv_blur.GetAddressOf(), m_depth_stencil_view.Get());

    for (DX11Mesh& cur_mesh : m_input.model.meshes)
    {
        cur_mesh.indices_buffer.Bind();
        cur_mesh.positions_buffer.BindToSlot(m_program.vs.geometry.POSITION);
        cur_mesh.texcoords_buffer.BindToSlot(m_program.vs.geometry.TEXCOORD);
        m_program_blur.ps.srv.ssaoInput.Attach(output.srv);
        m_context.device_context->DrawIndexed(cur_mesh.indices.size(), 0, 0);
    }

    m_context.device_context->OMSetRenderTargets(0, nullptr, nullptr);

    m_context.device_context->PSSetShaderResources(0, empty.size(), empty.data());
}

void SSAOPass::OnResize(int width, int height)
{
    m_width = width;
    m_height = height;
    CreateDsv(m_context, 1, m_width, m_height, m_depth_stencil_view);
    output.srv = CreateRtvSrv(m_context, 1, m_width, m_height, m_rtv);
    output.srv_blur = CreateRtvSrv(m_context, 1, m_width, m_height, m_rtv_blur);
}

void SSAOPass::OnModifySettings(const Settings& settings)
{
    Settings prev = m_settings;
    m_settings = settings;
    if (prev.msaa_count != m_settings.msaa_count)
    {
        m_program.ps.define["SAMPLE_COUNT"] = std::to_string(m_settings.msaa_count);
        m_program.ps.UpdateShader();
    }
}

void SSAOPass::SetDefines(Program<SSAOPassPS, SSAOPassVS>& program)
{
    program.ps.define["SAMPLE_COUNT"] = std::to_string(m_settings.msaa_count);
}

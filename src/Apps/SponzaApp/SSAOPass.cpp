#include "SSAOPass.h"
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
    CreateSizeDependentResources();

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

    std::vector<glm::vec4> ssaoNoise;
    for (uint32_t i = 0; i < 16; i++)
    {
        glm::vec4 noise(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, 0.0f, 0.0f);
        ssaoNoise.push_back(noise);
    }

    m_noise_texture = context.CreateTexture(BindFlag::kSrv, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, 4, 4, 1);
    size_t num_bytes = 0;
    size_t row_bytes = 0;
    GetSurfaceInfo(4, 4, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, &num_bytes, &row_bytes, nullptr);
    context.UpdateSubresource(m_noise_texture, 0, ssaoNoise.data(), row_bytes, num_bytes);
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

    m_program.SetMaxEvents(m_input.model.meshes.size());
    m_program_blur.SetMaxEvents(m_input.model.meshes.size());
}

void SSAOPass::OnRender()
{
    if (!m_settings.use_occlusion)
        return;

    m_context.SetViewport(m_width, m_height);

    m_program.UseProgram();

    float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_program.ps.om.rtv0.Attach(output.srv).Clear(color);
    m_program.ps.om.dsv.Attach(m_depth_stencil_view).Clear(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_input.model.ia.indices.Bind();
    m_input.model.ia.positions.BindToSlot(m_program.vs.ia.POSITION);
    m_input.model.ia.texcoords.BindToSlot(m_program.vs.ia.TEXCOORD);

    for (auto& range : m_input.model.ia.ranges)
    {
        m_program.ps.srv.gPosition.Attach(m_input.geometry_pass.position);
        m_program.ps.srv.gNormal.Attach(m_input.geometry_pass.normal);
        m_program.ps.srv.noiseTexture.Attach(m_noise_texture);
        m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
    }

    m_program_blur.UseProgram();
    m_program_blur.ps.uav.out_uav.Attach(output.srv_blur);
    m_program_blur.ps.om.dsv.Attach(m_depth_stencil_view).Clear(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_input.model.ia.indices.Bind();
    m_input.model.ia.positions.BindToSlot(m_program_blur.vs.ia.POSITION);
    m_input.model.ia.texcoords.BindToSlot(m_program_blur.vs.ia.TEXCOORD);
    for (auto& range : m_input.model.ia.ranges)
    {
        m_program_blur.ps.srv.ssaoInput.Attach(output.srv);
        m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
    }
}

void SSAOPass::OnResize(int width, int height)
{
    m_width = width;
    m_height = height;
    CreateSizeDependentResources();
}

void SSAOPass::CreateSizeDependentResources()
{
    output.srv = m_context.CreateTexture((BindFlag)(BindFlag::kRtv | BindFlag::kSrv), gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, m_width, m_height, 1);
    output.srv_blur = m_context.CreateTexture((BindFlag)(BindFlag::kRtv | BindFlag::kSrv | BindFlag::kUav), gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, m_width, m_height, 1);
    m_depth_stencil_view = m_context.CreateTexture((BindFlag)(BindFlag::kDsv), gli::format::FORMAT_D24_UNORM_S8_UINT_PACK32, 1, m_width, m_height, 1);
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

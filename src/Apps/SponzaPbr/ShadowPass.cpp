#include "ShadowPass.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

ShadowPass::ShadowPass(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_program(context)
{
    CreateSizeDependentResources();
    m_sampler = m_context.CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever });
}

void ShadowPass::OnUpdate()
{
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Down = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 Left = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 ForwardRH = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 ForwardLH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardRH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardLH = glm::vec3(0.0f, 0.0f, -1.0f);

    glm::vec3 position = m_input.light_pos;

    m_program.gs.cbuffer.GSParams.Projection = glm::transpose(glm::perspective(glm::radians(90.0f), 1.0f, m_settings.s_near, m_settings.s_far));

    std::array<glm::mat4, 6>& view = m_program.gs.cbuffer.GSParams.View;
    view[0] = glm::transpose(glm::lookAt(position, position + Right, Up));
    view[1] = glm::transpose(glm::lookAt(position, position + Left, Up));
    view[2] = glm::transpose(glm::lookAt(position, position + Up, BackwardRH));
    view[3] = glm::transpose(glm::lookAt(position, position + Down, ForwardRH));
    view[4] = glm::transpose(glm::lookAt(position, position + BackwardLH, Up));
    view[5] = glm::transpose(glm::lookAt(position, position + ForwardLH, Up));
}

void ShadowPass::OnRender()
{
    if (!m_settings.use_shadow)
        return;

    m_context.SetViewport(m_settings.s_size, m_settings.s_size);

    m_context.UseProgram(m_program);

    m_program.ps.sampler.g_sampler.Attach(m_sampler);

    std::array<float, 4> color = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_program.ps.om.dsv.Attach(output.srv).Clear(ClearFlag::kDepth | ClearFlag::kStencil, 1.0f, 0);

    for (auto& model : m_input.scene_list)
    {
        m_program.vs.cbuffer.VSParams.World = glm::transpose(model.matrix);

        m_program.SetRasterizeState({ FillMode::kSolid, CullMode::kBack, 4096 });

        model.ia.indices.Bind();
        model.ia.positions.BindToSlot(m_program.vs.ia.SV_POSITION);
        model.ia.texcoords.BindToSlot(m_program.vs.ia.TEXCOORD);

        for (auto& range : model.ia.ranges)
        {
            auto& material = model.GetMaterial(range.id);

            if (m_settings.shadow_discard)
                m_program.ps.srv.alphaMap.Attach(material.texture.opacity);
            else
                m_program.ps.srv.alphaMap.Attach();

            m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }
    }
}

void ShadowPass::OnResize(int width, int height)
{
}

void ShadowPass::CreateSizeDependentResources()
{
    output.srv = m_context.CreateTexture(BindFlag::kDsv | BindFlag::kSrv, gli::format::FORMAT_D32_SFLOAT_PACK32, 1, m_settings.s_size, m_settings.s_size, 6);
    m_buffer = m_context.CreateBuffer(BindFlag::kRtv, sizeof(float) * m_settings.s_size * m_settings.s_size, 0);
}

void ShadowPass::OnModifySponzaSettings(const SponzaSettings& settings)
{
    SponzaSettings prev = m_settings;
    m_settings = settings;
    if (prev.s_size != settings.s_size)
    {
        CreateSizeDependentResources();
    }
}

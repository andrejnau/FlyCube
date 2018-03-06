#include "ShadowPass.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <Utilities/State.h>

ShadowPass::ShadowPass(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program(context)
{
    output.srv = m_context.CreateTexture(BindFlag::kDsv | BindFlag::kSrv, DXGI_FORMAT_R32_TYPELESS, 1, m_settings.s_size, m_settings.s_size, 6);
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

    m_program.gs.cbuffer.Params.Projection = glm::transpose(glm::perspective(glm::radians(90.0f), 1.0f, m_settings.s_near, m_settings.s_far));

    std::array<glm::mat4, 6>& view = m_program.gs.cbuffer.Params.View;
    view[0] = glm::transpose(glm::lookAt(position, position + Right, Up));
    view[1] = glm::transpose(glm::lookAt(position, position + Left, Up));
    view[2] = glm::transpose(glm::lookAt(position, position + Up, BackwardRH));
    view[3] = glm::transpose(glm::lookAt(position, position + Down, ForwardRH));
    view[4] = glm::transpose(glm::lookAt(position, position + BackwardLH, Up));
    view[5] = glm::transpose(glm::lookAt(position, position + ForwardLH, Up));

    size_t cnt = 0;
    for (auto& scene_item : m_input.scene_list)
        for (auto& cur_mesh : scene_item.model.ia.ranges)
            ++cnt;
    m_program.SetMaxEvents(cnt);
}

void ShadowPass::OnRender()
{
    if (!m_settings.use_shadow)
        return;

    m_context.SetViewport(m_settings.s_size, m_settings.s_size);

    m_program.UseProgram();

    m_program.ps.sampler.g_sampler.Attach({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever });

    float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_context.OMSetRenderTargets({}, output.srv);
    m_context.ClearDepthStencil(output.srv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    auto& state = CurState<bool>::Instance().state;
    for (auto& scene_item : m_input.scene_list)
    {
        m_program.vs.cbuffer.Params.World = glm::transpose(scene_item.matrix);

        scene_item.model.bones.UpdateAnimation(glfwGetTime());

        Resource::Ptr bones_info_srv = scene_item.model.bones.GetBonesInfo(m_context);
        Resource::Ptr bone_srv = scene_item.model.bones.GetBone(m_context);

        m_program.vs.srv.bone_info.Attach(bones_info_srv);
        m_program.vs.srv.gBones.Attach(bone_srv);

        scene_item.model.ia.indices.Bind();
        scene_item.model.ia.positions.BindToSlot(m_program.vs.ia.SV_POSITION);
        scene_item.model.ia.texcoords.BindToSlot(m_program.vs.ia.TEXCOORD);
        scene_item.model.ia.bones_offset.BindToSlot(m_program.vs.ia.BONES_OFFSET);
        scene_item.model.ia.bones_count.BindToSlot(m_program.vs.ia.BONES_COUNT);

        for (auto& range : scene_item.model.ia.ranges)
        {
            auto& material = scene_item.model.ia.material[range.id];

            if (!state["no_shadow_discard"])
                m_program.ps.srv.alphaMap.Attach(material.GetTexture(aiTextureType_OPACITY));
            else
                m_program.ps.srv.alphaMap.Attach();

            m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }
    }
}

void ShadowPass::OnResize(int width, int height)
{
}

void ShadowPass::OnModifySettings(const Settings& settings)
{
    Settings prev = m_settings;
    m_settings = settings;
    if (prev.s_size != settings.s_size)
    {
        output.srv = m_context.CreateTexture(BindFlag::kDsv | BindFlag::kSrv, DXGI_FORMAT_R32_TYPELESS, 1, m_settings.s_size, m_settings.s_size, 6);
    }
}

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
}

void ShadowPass::OnRender()
{
    if (!m_settings.use_shadow)
        return;

    m_context.SetViewport(m_settings.s_size, m_settings.s_size);

    size_t cnt = 0;
    for (auto& scene_item : m_input.scene_list)
    {
        for (DX11Mesh& cur_mesh : scene_item.model.meshes)
        {
            ++cnt;
        }
    }

    m_program.UseProgram(cnt);

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
        m_program.vs.UpdateCBuffers();

        scene_item.model.bones.UpdateAnimation(glfwGetTime());

        ComPtr<IUnknown> bones_info_srv = m_context.CreateBuffer(BindFlag::kSrv, scene_item.model.bones.bone_info.size() * sizeof(Bones::BoneInfo), sizeof(Bones::BoneInfo), "bone_info");
        if (!scene_item.model.bones.bone_info.empty())
            m_context.UpdateSubresource(bones_info_srv, 0, scene_item.model.bones.bone_info.data(), 0, 0);

        ComPtr<IUnknown> bones_srv = m_context.CreateBuffer(BindFlag::kSrv, scene_item.model.bones.bone.size() * sizeof(glm::mat4), sizeof(glm::mat4), "bone");
        if (!scene_item.model.bones.bone.empty())
            m_context.UpdateSubresource(bones_srv, 0, scene_item.model.bones.bone.data(), 0, 0);

        m_program.vs.srv.bone_info.Attach(bones_info_srv);
        m_program.vs.srv.gBones.Attach(bones_srv);

        for (DX11Mesh& cur_mesh : scene_item.model.meshes)
        {
            cur_mesh.indices_buffer.Bind();
            cur_mesh.positions_buffer.BindToSlot(m_program.vs.ia.SV_POSITION);
            cur_mesh.texcoords_buffer.BindToSlot(m_program.vs.ia.TEXCOORD);
            cur_mesh.bones_offset_buffer.BindToSlot(m_program.vs.ia.BONES_OFFSET);
            cur_mesh.bones_count_buffer.BindToSlot(m_program.vs.ia.BONES_COUNT);

            if (!state["no_shadow_discard"])
                m_program.ps.srv.alphaMap.Attach(cur_mesh.GetTexture(aiTextureType_OPACITY));
            else
                m_program.ps.srv.alphaMap.Attach();

            m_program.ps.BindCBuffers();
            m_program.vs.BindCBuffers();
            m_program.gs.BindCBuffers();
            m_context.DrawIndexed(cur_mesh.indices.size());
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

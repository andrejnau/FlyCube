#include "IBLCompute.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <Utilities/State.h>

IBLCompute::IBLCompute(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_program(context)
    , m_program_downsample(context)
    , m_width(width)
    , m_height(height)
{
    CreateSizeDependentResources();
    m_sampler = m_context.CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever });
}

void IBLCompute::OnUpdate()
{
}

void IBLCompute::OnRender()
{
    m_context.SetViewport(m_size, m_size);

    m_program.UseProgram();

    m_program.ps.sampler.g_sampler.Attach(m_sampler);

    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Down = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 Left = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 ForwardRH = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 ForwardLH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardRH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardLH = glm::vec3(0.0f, 0.0f, -1.0f);

    m_program.gs.cbuffer.GSParams.Projection = glm::transpose(m_input.camera.GetProjectionMatrix());

    float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };

    for (auto& ibl_model : m_input.scene_list)
    {
        if (!ibl_model.ibl_request)
            continue;

        glm::vec3 position = ibl_model.matrix * glm::vec4(ibl_model.model_center, 1.0);
        std::array<glm::mat4, 6>& view = m_program.gs.cbuffer.GSParams.View;
        view[0] = glm::transpose(glm::lookAt(position, position + Right, Up));
        view[1] = glm::transpose(glm::lookAt(position, position + Left, Up));
        view[2] = glm::transpose(glm::lookAt(position, position + Up, BackwardRH));
        view[3] = glm::transpose(glm::lookAt(position, position + Down, ForwardRH));
        view[4] = glm::transpose(glm::lookAt(position, position + BackwardLH, Up));
        view[5] = glm::transpose(glm::lookAt(position, position + ForwardLH, Up));

        int texture_mips = 0;

        if (!ibl_model.ibl_rtv)
        {
            for (size_t i = 0; ; ++i)
            {
                if ((m_size >> i) % 8 != 0)
                    break;
                ++texture_mips;
            }
            ibl_model.ibl_rtv = m_context.CreateTexture(BindFlag::kRtv | BindFlag::kSrv | BindFlag::kUav,
                gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, m_size, m_size, 6, texture_mips);
        }

        m_program.ps.om.rtv0.Attach(ibl_model.ibl_rtv).Clear(color);
        m_program.ps.om.dsv.Attach(m_dsv).Clear(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        for (auto& model : m_input.scene_list)
        {
            if (&ibl_model == &model)
                continue;

            m_program.vs.cbuffer.ConstantBuf.model = glm::transpose(model.matrix);
            m_program.vs.cbuffer.ConstantBuf.normalMatrix = glm::transpose(glm::transpose(glm::inverse(model.matrix)));

            model.bones.UpdateAnimation(glfwGetTime());

            Resource::Ptr bones_info_srv = model.bones.GetBonesInfo(m_context);
            Resource::Ptr bone_srv = model.bones.GetBone(m_context);

            m_program.vs.srv.bone_info.Attach(bones_info_srv);
            m_program.vs.srv.gBones.Attach(bone_srv);

            model.ia.indices.Bind();
            model.ia.positions.BindToSlot(m_program.vs.ia.POSITION);
            model.ia.normals.BindToSlot(m_program.vs.ia.NORMAL);
            model.ia.texcoords.BindToSlot(m_program.vs.ia.TEXCOORD);
            model.ia.tangents.BindToSlot(m_program.vs.ia.TANGENT);
            model.ia.bones_offset.BindToSlot(m_program.vs.ia.BONES_OFFSET);
            model.ia.bones_count.BindToSlot(m_program.vs.ia.BONES_COUNT);

            for (auto& range : model.ia.ranges)
            {
                auto& material = model.GetMaterial(range.id);

                /*m_program.ps.cbuffer.Settings.use_normal_mapping = material.texture.normal && m_settings.normal_mapping;
                m_program.ps.cbuffer.Settings.use_gloss_instead_of_roughness = material.texture.glossiness && !material.texture.roughness;
                m_program.ps.cbuffer.Settings.use_flip_normal_y = m_settings.use_flip_normal_y;*/

                //m_program.ps.srv.normalMap.Attach(material.texture.normal);
                m_program.ps.srv.albedoMap.Attach(material.texture.albedo);
               /*m_program.ps.srv.glossMap.Attach(material.texture.glossiness);
                m_program.ps.srv.roughnessMap.Attach(material.texture.roughness);
                m_program.ps.srv.metalnessMap.Attach(material.texture.metalness);
                m_program.ps.srv.aoMap.Attach(material.texture.occlusion);*/
                m_program.ps.srv.alphaMap.Attach(material.texture.opacity);

                m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
            }
        }

        m_program_downsample.UseProgram();
        for (size_t i = 1; i < texture_mips; ++i)
        {
            m_program_downsample.cs.srv.inputTexture.Attach(ibl_model.ibl_rtv, i - 1);
            m_program_downsample.cs.uav.outputTexture.Attach(ibl_model.ibl_rtv, i);
            m_context.Dispatch((texture_mips >> i) / 8, (texture_mips >> i) / 8, 6);
        }
    }
}

void IBLCompute::OnResize(int width, int height)
{
}

void IBLCompute::CreateSizeDependentResources()
{
    m_dsv = m_context.CreateTexture(BindFlag::kDsv, gli::format::FORMAT_D32_SFLOAT_PACK32, 1, m_size, m_size, 6);
}

void IBLCompute::OnModifySettings(const Settings& settings)
{
    Settings prev = m_settings;
    m_settings = settings;
    if (prev.s_size != settings.s_size)
    {
        CreateSizeDependentResources();
    }
}

#include "IBLCompute.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

IBLCompute::IBLCompute(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_program(context)
    , m_program_pre_pass(context)
    , m_program_backgroud(context)
    , m_program_downsample(context)
    , m_width(width)
    , m_height(height)
{
    m_sampler = m_context.CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever });

    m_compare_sampler = m_context.CreateSampler({
        SamplerFilter::kComparisonMinMagMipLinear,
        SamplerTextureAddressMode::kClamp,
        SamplerComparisonFunc::kLess });
}

void IBLCompute::OnUpdate()
{
    glm::vec3 camera_position = m_input.camera.GetCameraPos();
    m_program.ps.cbuffer.Light.viewPos = glm::vec4(camera_position, 0.0);

    m_program.ps.cbuffer.ShadowParams.s_near = m_settings.s_near;
    m_program.ps.cbuffer.ShadowParams.s_far = m_settings.s_far;
    m_program.ps.cbuffer.ShadowParams.s_size = m_settings.s_size;
    m_program.ps.cbuffer.ShadowParams.use_shadow = m_settings.use_shadow;
    m_program.ps.cbuffer.ShadowParams.shadow_light_pos = m_input.light_pos;

    m_program.ps.cbuffer.Settings.ambient_power = m_settings.ambient_power;
    m_program.ps.cbuffer.Settings.light_power = m_settings.light_power;

    m_program.ps.cbuffer.Light.use_light = m_use_pre_pass;

    for (size_t i = 0; i < std::size(m_program.ps.cbuffer.Light.light_pos); ++i)
    {
        m_program.ps.cbuffer.Light.light_pos[i] = glm::vec4(0);
        m_program.ps.cbuffer.Light.light_color[i] = glm::vec4(0);
    }

    if (m_settings.light_in_camera)
    {
        m_program.ps.cbuffer.Light.light_pos[0] = glm::vec4(camera_position, 0);
        m_program.ps.cbuffer.Light.light_color[0] = glm::vec4(1, 1, 1, 0.0);
    }
    if (m_settings.additional_lights)
    {
        int i = 0;
        if (m_settings.light_in_camera)
            ++i;
        for (int x = -13; x <= 13; ++x)
        {
            int q = 1;
            for (int z = -1; z <= 1; ++z)
            {
                if (i < std::size(m_program.ps.cbuffer.Light.light_pos))
                {
                    m_program.ps.cbuffer.Light.light_pos[i] = glm::vec4(x, 1.5, z - 0.33, 0);
                    float color = 0.0;
                    if (m_settings.use_white_ligth)
                        color = 1;
                    m_program.ps.cbuffer.Light.light_color[i] = glm::vec4(q == 1 ? 1 : color, q == 2 ? 1 : color, q == 3 ? 1 : color, 0.0);
                    ++i;
                    ++q;
                }
            }
        }
    }
}

void IBLCompute::OnRender()
{
    m_context.SetViewport(m_size, m_size);

    for (auto& ibl_model : m_input.scene_list)
    {
        if (!ibl_model.ibl_request)
            continue;

        size_t texture_mips = 0;
        for (size_t i = 0; ; ++i)
        {
            if ((m_size >> i) % 8 != 0)
                break;
            ++texture_mips;
        }

        if (!ibl_model.ibl_rtv)
        {
            ibl_model.ibl_rtv = m_context.CreateTexture(BindFlag::kRenderTarget | BindFlag::kShaderResource | BindFlag::kUnorderedAccess,
                gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, m_size, m_size, 6, texture_mips);

            ibl_model.ibl_dsv = m_context.CreateTexture(BindFlag::kDepthStencil, gli::format::FORMAT_D32_SFLOAT_PACK32, 1, m_size, m_size, 6);
        }
        else
        {
            continue;
        }

        if (m_use_pre_pass)
            DrawPrePass(ibl_model);
        Draw(ibl_model);
        DrawBackgroud(ibl_model);
        DrawDownSample(ibl_model, texture_mips);
    }
}

void IBLCompute::DrawPrePass(Model & ibl_model)
{
    m_context.UseProgram(m_program_pre_pass);

    m_program_pre_pass.ps.sampler.g_sampler.Attach(m_sampler);

    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Down = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 Left = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 ForwardRH = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 ForwardLH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardRH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardLH = glm::vec3(0.0f, 0.0f, -1.0f);

    m_program_pre_pass.gs.cbuffer.GSParams.Projection = glm::transpose(glm::perspective(glm::radians(90.0f), 1.0f, m_settings.s_near, m_settings.s_far));

    glm::vec3 position = ibl_model.matrix * glm::vec4(ibl_model.model_center, 1.0);
    std::array<glm::mat4, 6>& view = m_program_pre_pass.gs.cbuffer.GSParams.View;
    view[0] = glm::transpose(glm::lookAt(position, position + Right, Up));
    view[1] = glm::transpose(glm::lookAt(position, position + Left, Up));
    view[2] = glm::transpose(glm::lookAt(position, position + Up, BackwardRH));
    view[3] = glm::transpose(glm::lookAt(position, position + Down, ForwardRH));
    view[4] = glm::transpose(glm::lookAt(position, position + BackwardLH, Up));
    view[5] = glm::transpose(glm::lookAt(position, position + ForwardLH, Up));

    m_program_pre_pass.ps.om.dsv.Attach(ibl_model.ibl_dsv).Clear(ClearFlag::kDepth | ClearFlag::kStencil, 1.0f, 0);

    for (auto& model : m_input.scene_list)
    {
        if (&ibl_model == &model)
            continue;

        m_program_pre_pass.vs.cbuffer.ConstantBuf.model = glm::transpose(model.matrix);
        m_program_pre_pass.vs.cbuffer.ConstantBuf.normalMatrix = glm::transpose(glm::transpose(glm::inverse(model.matrix)));

        model.bones.UpdateAnimation(glfwGetTime());

        std::shared_ptr<Resource> bones_info_srv = model.bones.GetBonesInfo(m_context);
        std::shared_ptr<Resource> bone_srv = model.bones.GetBone(m_context);

        m_program_pre_pass.vs.srv.bone_info.Attach(bones_info_srv);
        m_program_pre_pass.vs.srv.gBones.Attach(bone_srv);

        model.ia.indices.Bind();
        model.ia.positions.BindToSlot(m_program_pre_pass.vs.ia.POSITION);
        model.ia.normals.BindToSlot(m_program_pre_pass.vs.ia.NORMAL);
        model.ia.texcoords.BindToSlot(m_program_pre_pass.vs.ia.TEXCOORD);
        model.ia.tangents.BindToSlot(m_program_pre_pass.vs.ia.TANGENT);
        model.ia.bones_offset.BindToSlot(m_program_pre_pass.vs.ia.BONES_OFFSET);
        model.ia.bones_count.BindToSlot(m_program_pre_pass.vs.ia.BONES_COUNT);

        for (auto& range : model.ia.ranges)
        {
            auto& material = model.GetMaterial(range.id);
            m_program_pre_pass.ps.srv.alphaMap.Attach(material.texture.opacity);
            m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }
    }
}

void IBLCompute::Draw(Model& ibl_model)
{
    m_context.UseProgram(m_program);

    m_program.ps.sampler.g_sampler.Attach(m_sampler);
    m_program.ps.sampler.LightCubeShadowComparsionSampler.Attach(m_compare_sampler);

    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Down = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 Left = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 ForwardRH = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 ForwardLH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardRH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardLH = glm::vec3(0.0f, 0.0f, -1.0f);

    m_program.gs.cbuffer.GSParams.Projection = glm::transpose(glm::perspective(glm::radians(90.0f), 1.0f, m_settings.s_near, m_settings.s_far));

    std::array<float, 4> color = { 0.0f, 0.0f, 0.0f, 1.0f };

    glm::vec3 position = ibl_model.matrix * glm::vec4(ibl_model.model_center, 1.0);
    std::array<glm::mat4, 6>& view = m_program.gs.cbuffer.GSParams.View;
    view[0] = glm::transpose(glm::lookAt(position, position + Right, Up));
    view[1] = glm::transpose(glm::lookAt(position, position + Left, Up));
    view[2] = glm::transpose(glm::lookAt(position, position + Up, BackwardRH));
    view[3] = glm::transpose(glm::lookAt(position, position + Down, ForwardRH));
    view[4] = glm::transpose(glm::lookAt(position, position + BackwardLH, Up));
    view[5] = glm::transpose(glm::lookAt(position, position + ForwardLH, Up));

    m_program.ps.om.rtv0.Attach(ibl_model.ibl_rtv).Clear(color);
    if (m_use_pre_pass)
    {
        m_program.ps.om.dsv.Attach(ibl_model.ibl_dsv);
        m_program.SetDepthStencilState({ true, DepthComparison::kLessEqual });
    }
    else
    {
        m_program.ps.om.dsv.Attach(ibl_model.ibl_dsv).Clear(ClearFlag::kDepth | ClearFlag::kStencil, 1.0f, 0);
    }

    for (auto& model : m_input.scene_list)
    {
        if (&ibl_model == &model)
            continue;

        m_program.vs.cbuffer.ConstantBuf.model = glm::transpose(model.matrix);
        m_program.vs.cbuffer.ConstantBuf.normalMatrix = glm::transpose(glm::transpose(glm::inverse(model.matrix)));

        model.bones.UpdateAnimation(glfwGetTime());

        std::shared_ptr<Resource> bones_info_srv = model.bones.GetBonesInfo(m_context);
        std::shared_ptr<Resource> bone_srv = model.bones.GetBone(m_context);

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

            m_program.ps.cbuffer.Settings.use_normal_mapping = material.texture.normal && m_settings.normal_mapping;
            m_program.ps.cbuffer.Settings.use_gloss_instead_of_roughness = material.texture.glossiness && !material.texture.roughness;
            m_program.ps.cbuffer.Settings.use_flip_normal_y = m_settings.use_flip_normal_y;

            m_program.ps.srv.normalMap.Attach(material.texture.normal);
            m_program.ps.srv.albedoMap.Attach(material.texture.albedo);
            m_program.ps.srv.roughnessMap.Attach(material.texture.roughness);
            m_program.ps.srv.metalnessMap.Attach(material.texture.metalness);
            m_program.ps.srv.aoMap.Attach(material.texture.occlusion);
            m_program.ps.srv.alphaMap.Attach(material.texture.opacity);
            m_program.ps.srv.LightCubeShadowMap.Attach(m_input.shadow_pass.srv);

            m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }
    }
}

void IBLCompute::DrawBackgroud(Model& ibl_model)
{
    m_context.UseProgram(m_program_backgroud);

    m_program_backgroud.SetDepthStencilState({ true, DepthComparison::kLessEqual });

    m_program_backgroud.ps.sampler.g_sampler.Attach(m_sampler);

    m_program_backgroud.ps.om.rtv0.Attach(ibl_model.ibl_rtv);
    m_program_backgroud.ps.om.dsv.Attach(ibl_model.ibl_dsv);

    m_input.model_cube.ia.indices.Bind();
    m_input.model_cube.ia.positions.BindToSlot(m_program_backgroud.vs.ia.POSITION);

    m_program_backgroud.ps.srv.environmentMap.Attach(m_input.environment);

    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Down = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 Left = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 ForwardRH = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 ForwardLH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardRH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardLH = glm::vec3(0.0f, 0.0f, -1.0f);

    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::mat4 capture_views[] =
    {
        glm::lookAt(position, position + Right, Up),
        glm::lookAt(position, position + Left, Up),
        glm::lookAt(position, position + Up, BackwardRH),
        glm::lookAt(position, position + Down, ForwardRH),
        glm::lookAt(position, position + BackwardLH, Up),
        glm::lookAt(position, position + ForwardLH, Up)
    };

    for (uint32_t i = 0; i < 6; ++i)
    {
        m_program_backgroud.vs.cbuffer.ConstantBuf.face = i;
        m_program_backgroud.vs.cbuffer.ConstantBuf.view = glm::transpose(capture_views[i]);
        m_program_backgroud.vs.cbuffer.ConstantBuf.projection = glm::transpose(glm::perspective(glm::radians(90.0f), 1.0f, m_settings.s_near, m_settings.s_far));

        for (auto& range : m_input.model_cube.ia.ranges)
        {
            m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }
    }
}

void IBLCompute::DrawDownSample(Model & ibl_model, size_t texture_mips)
{
    m_context.UseProgram(m_program_downsample);
    for (size_t i = 1; i < texture_mips; ++i)
    {
        m_program_downsample.cs.srv.inputTexture.Attach(ibl_model.ibl_rtv, {i - 1, 1});
        m_program_downsample.cs.uav.outputTexture.Attach(ibl_model.ibl_rtv, {i, 1});
        m_context.Dispatch((m_size >> i) / 8, (m_size >> i) / 8, 6);
    }
}

void IBLCompute::OnResize(int width, int height)
{
}

void IBLCompute::OnModifySponzaSettings(const SponzaSettings& settings)
{
    m_settings = settings;
}

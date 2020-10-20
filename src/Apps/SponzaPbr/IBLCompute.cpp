#include "IBLCompute.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

IBLCompute::IBLCompute(Device& device, const Input& input)
    : m_device(device)
    , m_input(input)
    , m_program(device)
    , m_program_pre_pass(device)
    , m_program_backgroud(device)
    , m_program_downsample(device)
{
    m_sampler = m_device.CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever });

    m_compare_sampler = m_device.CreateSampler({
        SamplerFilter::kComparisonMinMagMipLinear,
        SamplerTextureAddressMode::kClamp,
        SamplerComparisonFunc::kLess });
}

void IBLCompute::OnUpdate()
{
    glm::vec3 camera_position = m_input.camera.GetCameraPos();
    m_program.ps.cbuffer.Light.viewPos = camera_position;

    m_program.ps.cbuffer.ShadowParams.s_near = m_settings.Get<float>("s_near");
    m_program.ps.cbuffer.ShadowParams.s_far = m_settings.Get<float>("s_far");
    m_program.ps.cbuffer.ShadowParams.s_size = m_settings.Get<float>("s_size");
    m_program.ps.cbuffer.ShadowParams.use_shadow = m_settings.Get<bool>("use_shadow");
    m_program.ps.cbuffer.ShadowParams.shadow_light_pos = m_input.light_pos;

    m_program.ps.cbuffer.Settings.ambient_power = m_settings.Get<float>("ambient_power");
    m_program.ps.cbuffer.Settings.light_power = m_settings.Get<float>("light_power");

    m_program.ps.cbuffer.Light.use_light = m_use_pre_pass;

    for (size_t i = 0; i < std::size(m_program.ps.cbuffer.Light.light_pos); ++i)
    {
        m_program.ps.cbuffer.Light.light_pos[i] = glm::vec4(0);
        m_program.ps.cbuffer.Light.light_color[i] = glm::vec4(0);
    }

    if (m_settings.Get<bool>("light_in_camera"))
    {
        m_program.ps.cbuffer.Light.light_pos[0] = glm::vec4(camera_position, 0);
        m_program.ps.cbuffer.Light.light_color[0] = glm::vec4(1, 1, 1, 0.0);
    }
    if (m_settings.Get<bool>("additional_lights"))
    {
        int i = 0;
        if (m_settings.Get<bool>("light_in_camera"))
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
                    if (m_settings.Get<bool>("use_white_ligth"))
                        color = 1;
                    m_program.ps.cbuffer.Light.light_color[i] = glm::vec4(q == 1 ? 1 : color, q == 2 ? 1 : color, q == 3 ? 1 : color, 0.0);
                    ++i;
                    ++q;
                }
            }
        }
    }
}

void IBLCompute::OnRender(RenderCommandList& command_list)
{
    command_list.SetViewport(0, 0, m_size, m_size);

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
            ibl_model.ibl_rtv = m_device.CreateTexture(BindFlag::kRenderTarget | BindFlag::kShaderResource | BindFlag::kUnorderedAccess,
                gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, m_size, m_size, 6, texture_mips);

            ibl_model.ibl_dsv = m_device.CreateTexture(BindFlag::kDepthStencil, gli::format::FORMAT_D32_SFLOAT_PACK32, 1, m_size, m_size, 6);
        }
        else
        {
            continue;
        }

        if (m_use_pre_pass)
            DrawPrePass(command_list, ibl_model);
        Draw(command_list, ibl_model);
        DrawBackgroud(command_list, ibl_model);
        DrawDownSample(command_list, ibl_model, texture_mips);
    }
}

void IBLCompute::DrawPrePass(RenderCommandList& command_list, Model & ibl_model)
{
    command_list.UseProgram(m_program_pre_pass);
    command_list.Attach(m_program_pre_pass.vs.cbv.ConstantBuf, m_program_pre_pass.vs.cbuffer.ConstantBuf);
    command_list.Attach(m_program_pre_pass.gs.cbv.GSParams, m_program_pre_pass.gs.cbuffer.GSParams);

    command_list.Attach(m_program_pre_pass.ps.sampler.g_sampler, m_sampler);

    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Down = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 Left = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 ForwardRH = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 ForwardLH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardRH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardLH = glm::vec3(0.0f, 0.0f, -1.0f);

    m_program_pre_pass.gs.cbuffer.GSParams.Projection = glm::transpose(glm::perspective(glm::radians(90.0f), 1.0f, m_settings.Get<float>("s_near"), m_settings.Get<float>("s_far")));

    glm::vec3 position = glm::vec3(ibl_model.matrix * glm::vec4(ibl_model.model_center, 1.0));
    std::array<glm::mat4, 6>& view = m_program_pre_pass.gs.cbuffer.GSParams.View;
    view[0] = glm::transpose(glm::lookAt(position, position + Right, Up));
    view[1] = glm::transpose(glm::lookAt(position, position + Left, Up));
    view[2] = glm::transpose(glm::lookAt(position, position + Up, BackwardRH));
    view[3] = glm::transpose(glm::lookAt(position, position + Down, ForwardRH));
    view[4] = glm::transpose(glm::lookAt(position, position + BackwardLH, Up));
    view[5] = glm::transpose(glm::lookAt(position, position + ForwardLH, Up));

    RenderPassBeginDesc render_pass_desc = {};
    render_pass_desc.depth_stencil.texture = ibl_model.ibl_dsv;
    render_pass_desc.depth_stencil.clear_depth = 1.0f;

    for (auto& model : m_input.scene_list)
    {
        if (&ibl_model == &model)
            continue;
        model.bones.UpdateAnimation(m_device, command_list, glfwGetTime());
    }

    command_list.BeginRenderPass(render_pass_desc);
    for (auto& model : m_input.scene_list)
    {
        if (&ibl_model == &model)
            continue;

        m_program_pre_pass.vs.cbuffer.ConstantBuf.model = glm::transpose(model.matrix);
        m_program_pre_pass.vs.cbuffer.ConstantBuf.normalMatrix = glm::transpose(glm::transpose(glm::inverse(model.matrix)));

        std::shared_ptr<Resource> bones_info_srv = model.bones.GetBonesInfo();
        std::shared_ptr<Resource> bone_srv = model.bones.GetBone();

        command_list.Attach(m_program_pre_pass.vs.srv.bone_info, bones_info_srv);
        command_list.Attach(m_program_pre_pass.vs.srv.gBones, bone_srv);

        model.ia.indices.Bind(command_list);
        model.ia.positions.BindToSlot(command_list, m_program_pre_pass.vs.ia.POSITION);
        model.ia.normals.BindToSlot(command_list, m_program_pre_pass.vs.ia.NORMAL);
        model.ia.texcoords.BindToSlot(command_list, m_program_pre_pass.vs.ia.TEXCOORD);
        model.ia.tangents.BindToSlot(command_list, m_program_pre_pass.vs.ia.TANGENT);
        model.ia.bones_offset.BindToSlot(command_list, m_program_pre_pass.vs.ia.BONES_OFFSET);
        model.ia.bones_count.BindToSlot(command_list, m_program_pre_pass.vs.ia.BONES_COUNT);

        for (auto& range : model.ia.ranges)
        {
            auto& material = model.GetMaterial(range.id);
            command_list.Attach(m_program_pre_pass.ps.srv.alphaMap, material.texture.opacity);
            command_list.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }
    }
    command_list.EndRenderPass();
}

void IBLCompute::Draw(RenderCommandList& command_list, Model& ibl_model)
{
    command_list.UseProgram(m_program);
    command_list.Attach(m_program.vs.cbv.ConstantBuf, m_program.vs.cbuffer.ConstantBuf);
    command_list.Attach(m_program.gs.cbv.GSParams, m_program.gs.cbuffer.GSParams);
    command_list.Attach(m_program.ps.cbv.Light, m_program.ps.cbuffer.Light);
    command_list.Attach(m_program.ps.cbv.ShadowParams, m_program.ps.cbuffer.ShadowParams);
    command_list.Attach(m_program.ps.cbv.Settings, m_program.ps.cbuffer.Settings);

    command_list.Attach(m_program.ps.sampler.g_sampler, m_sampler);
    command_list.Attach(m_program.ps.sampler.LightCubeShadowComparsionSampler, m_compare_sampler);

    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Down = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 Left = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 ForwardRH = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 ForwardLH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardRH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardLH = glm::vec3(0.0f, 0.0f, -1.0f);

    m_program.gs.cbuffer.GSParams.Projection = glm::transpose(glm::perspective(glm::radians(90.0f), 1.0f, m_settings.Get<float>("s_near"), m_settings.Get<float>("s_far")));

    glm::vec4 color = { 0.0f, 0.0f, 0.0f, 1.0f };

    glm::vec3 position = glm::vec3(ibl_model.matrix * glm::vec4(ibl_model.model_center, 1.0));
    std::array<glm::mat4, 6>& view = m_program.gs.cbuffer.GSParams.View;
    view[0] = glm::transpose(glm::lookAt(position, position + Right, Up));
    view[1] = glm::transpose(glm::lookAt(position, position + Left, Up));
    view[2] = glm::transpose(glm::lookAt(position, position + Up, BackwardRH));
    view[3] = glm::transpose(glm::lookAt(position, position + Down, ForwardRH));
    view[4] = glm::transpose(glm::lookAt(position, position + BackwardLH, Up));
    view[5] = glm::transpose(glm::lookAt(position, position + ForwardLH, Up));

    RenderPassBeginDesc render_pass_desc = {};
    render_pass_desc.colors[m_program.ps.om.rtv0].texture = ibl_model.ibl_rtv;
    render_pass_desc.colors[m_program.ps.om.rtv0].clear_color = color;
    if (m_use_pre_pass)
    {
        render_pass_desc.depth_stencil.texture = ibl_model.ibl_dsv;
        render_pass_desc.depth_stencil.depth_load_op = RenderPassLoadOp::kLoad;
        command_list.SetDepthStencilState({ true, DepthComparison::kLessEqual });
    }
    else
    {
        render_pass_desc.depth_stencil.texture = ibl_model.ibl_dsv;
        render_pass_desc.depth_stencil.clear_depth = 1.0f;
    }

    for (auto& model : m_input.scene_list)
    {
        if (&ibl_model == &model)
            continue;
        model.bones.UpdateAnimation(m_device, command_list, glfwGetTime());
    }

    command_list.BeginRenderPass(render_pass_desc);
    for (auto& model : m_input.scene_list)
    {
        if (&ibl_model == &model)
            continue;

        m_program.vs.cbuffer.ConstantBuf.model = glm::transpose(model.matrix);
        m_program.vs.cbuffer.ConstantBuf.normalMatrix = glm::transpose(glm::transpose(glm::inverse(model.matrix)));

        std::shared_ptr<Resource> bones_info_srv = model.bones.GetBonesInfo();
        std::shared_ptr<Resource> bone_srv = model.bones.GetBone();

        command_list.Attach(m_program.vs.srv.bone_info, bones_info_srv);
        command_list.Attach(m_program.vs.srv.gBones, bone_srv);

        model.ia.indices.Bind(command_list);
        model.ia.positions.BindToSlot(command_list, m_program.vs.ia.POSITION);
        model.ia.normals.BindToSlot(command_list, m_program.vs.ia.NORMAL);
        model.ia.texcoords.BindToSlot(command_list, m_program.vs.ia.TEXCOORD);
        model.ia.tangents.BindToSlot(command_list, m_program.vs.ia.TANGENT);
        model.ia.bones_offset.BindToSlot(command_list, m_program.vs.ia.BONES_OFFSET);
        model.ia.bones_count.BindToSlot(command_list, m_program.vs.ia.BONES_COUNT);

        for (auto& range : model.ia.ranges)
        {
            auto& material = model.GetMaterial(range.id);

            m_program.ps.cbuffer.Settings.use_normal_mapping = material.texture.normal && m_settings.Get<bool>("normal_mapping");
            m_program.ps.cbuffer.Settings.use_gloss_instead_of_roughness = material.texture.glossiness && !material.texture.roughness;
            m_program.ps.cbuffer.Settings.use_flip_normal_y = m_settings.Get<bool>("use_flip_normal_y");

            command_list.Attach(m_program.ps.srv.normalMap, material.texture.normal);
            command_list.Attach(m_program.ps.srv.albedoMap, material.texture.albedo);
            command_list.Attach(m_program.ps.srv.roughnessMap, material.texture.roughness);
            command_list.Attach(m_program.ps.srv.metalnessMap, material.texture.metalness);
            command_list.Attach(m_program.ps.srv.aoMap, material.texture.occlusion);
            command_list.Attach(m_program.ps.srv.alphaMap, material.texture.opacity);
            command_list.Attach(m_program.ps.srv.LightCubeShadowMap, m_input.shadow_pass.srv);

            command_list.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }
    }
    command_list.EndRenderPass();
}

void IBLCompute::DrawBackgroud(RenderCommandList& command_list, Model& ibl_model)
{
    command_list.UseProgram(m_program_backgroud);
    command_list.Attach(m_program_backgroud.vs.cbv.ConstantBuf, m_program_backgroud.vs.cbuffer.ConstantBuf);

    command_list.SetDepthStencilState({ true, DepthComparison::kLessEqual });

    command_list.Attach(m_program_backgroud.ps.sampler.g_sampler, m_sampler);

    RenderPassBeginDesc render_pass_desc = {};
    render_pass_desc.colors[m_program_backgroud.ps.om.rtv0].texture = ibl_model.ibl_rtv;
    render_pass_desc.colors[m_program_backgroud.ps.om.rtv0].load_op = RenderPassLoadOp::kLoad;
    render_pass_desc.depth_stencil.texture = ibl_model.ibl_dsv;
    render_pass_desc.depth_stencil.depth_load_op = RenderPassLoadOp::kLoad;

    m_input.model_cube.ia.indices.Bind(command_list);
    m_input.model_cube.ia.positions.BindToSlot(command_list, m_program_backgroud.vs.ia.POSITION);

    command_list.Attach(m_program_backgroud.ps.srv.environmentMap, m_input.environment);

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

    command_list.BeginRenderPass(render_pass_desc);
    for (uint32_t i = 0; i < 6; ++i)
    {
        m_program_backgroud.vs.cbuffer.ConstantBuf.face = i;
        m_program_backgroud.vs.cbuffer.ConstantBuf.view = glm::transpose(capture_views[i]);
        m_program_backgroud.vs.cbuffer.ConstantBuf.projection = glm::transpose(glm::perspective(glm::radians(90.0f), 1.0f, m_settings.Get<float>("s_near"), m_settings.Get<float>("s_far")));

        for (auto& range : m_input.model_cube.ia.ranges)
        {
            command_list.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }
    }
    command_list.EndRenderPass();
}

void IBLCompute::DrawDownSample(RenderCommandList& command_list, Model& ibl_model, size_t texture_mips)
{
    command_list.UseProgram(m_program_downsample);
    for (size_t i = 1; i < texture_mips; ++i)
    {
        command_list.Attach(m_program_downsample.cs.srv.inputTexture, ibl_model.ibl_rtv, {i - 1, 1});
        command_list.Attach(m_program_downsample.cs.uav.outputTexture, ibl_model.ibl_rtv, {i, 1});
        command_list.Dispatch((m_size >> i) / 8, (m_size >> i) / 8, 6);
    }
}

void IBLCompute::OnModifySponzaSettings(const SponzaSettings& settings)
{
    m_settings = settings;
}

#include "GeometryPass.h"

#include <glm/gtx/transform.hpp>

GeometryPass::GeometryPass(Device& device, const Input& input, int width, int height)
    : m_device(device)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program(device)
{
    CreateSizeDependentResources();
    m_sampler = m_device.CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever });
}

void GeometryPass::OnUpdate()
{
    glm::mat4 projection, view, model;
    m_input.camera.GetMatrix(projection, view, model);
    
    m_program.vs.cbuffer.ConstantBuf.view = glm::transpose(view);
    m_program.vs.cbuffer.ConstantBuf.projection = glm::transpose(projection);
}

void GeometryPass::OnRender(CommandListBox& command_list)
{
    command_list.SetViewport(m_width, m_height);

    command_list.UseProgram(m_program);
    command_list.Attach(m_program.vs.cbv.ConstantBuf, m_program.vs.cbuffer.ConstantBuf);
    command_list.Attach(m_program.ps.cbv.Settings, m_program.ps.cbuffer.Settings);

    command_list.Attach(m_program.ps.sampler.g_sampler, m_sampler);

    std::array<float, 4> color = { 0.0f, 0.0f, 0.0f, 1.0f };
    command_list.Attach(m_program.ps.om.rtv0, output.position);
    command_list.ClearColor(m_program.ps.om.rtv0, color);
    command_list.Attach(m_program.ps.om.rtv1, output.normal);
    command_list.ClearColor(m_program.ps.om.rtv1, color);
    command_list.Attach(m_program.ps.om.rtv2, output.albedo);
    command_list.ClearColor(m_program.ps.om.rtv2, color);
    command_list.Attach(m_program.ps.om.rtv3, output.material);
    command_list.ClearColor(m_program.ps.om.rtv3, color);
    command_list.Attach(m_program.ps.om.dsv, output.dsv);
    command_list.ClearDepth(m_program.ps.om.dsv, 1.0f);

    bool skiped = false;
    for (auto& model : m_input.scene_list)
    {
        if (!skiped && m_settings.Get<bool>("skip_sponza_model"))
        {
            skiped = true;
            continue;
        }
        m_program.vs.cbuffer.ConstantBuf.model = glm::transpose(model.matrix);
        m_program.vs.cbuffer.ConstantBuf.normalMatrix = glm::transpose(glm::transpose(glm::inverse(model.matrix)));
        m_program.ps.cbuffer.Settings.ibl_source = model.ibl_source;

        model.ia.indices.Bind(command_list);
        model.ia.positions.BindToSlot(command_list, m_program.vs.ia.POSITION);
        model.ia.normals.BindToSlot(command_list, m_program.vs.ia.NORMAL);
        model.ia.texcoords.BindToSlot(command_list, m_program.vs.ia.TEXCOORD);
        model.ia.tangents.BindToSlot(command_list, m_program.vs.ia.TANGENT);

        for (auto& range : model.ia.ranges)
        {
            auto& material = model.GetMaterial(range.id);

            m_program.ps.cbuffer.Settings.use_normal_mapping = material.texture.normal && m_settings.Get<bool>("normal_mapping");
            m_program.ps.cbuffer.Settings.use_gloss_instead_of_roughness = material.texture.glossiness && !material.texture.roughness;
            m_program.ps.cbuffer.Settings.use_flip_normal_y = m_settings.Get<bool>("use_flip_normal_y");

            command_list.Attach(m_program.ps.srv.normalMap, material.texture.normal);
            command_list.Attach(m_program.ps.srv.albedoMap, material.texture.albedo);
            command_list.Attach(m_program.ps.srv.glossMap, material.texture.glossiness);
            command_list.Attach(m_program.ps.srv.roughnessMap, material.texture.roughness);
            command_list.Attach(m_program.ps.srv.metalnessMap, material.texture.metalness);
            command_list.Attach(m_program.ps.srv.aoMap, material.texture.occlusion);
            command_list.Attach(m_program.ps.srv.alphaMap, material.texture.opacity);

            command_list.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }
    }
}

void GeometryPass::OnResize(int width, int height)
{
    m_width = width;
    m_height = height;
    CreateSizeDependentResources();
}

void GeometryPass::OnModifySponzaSettings(const SponzaSettings& settings)
{
    SponzaSettings prev = m_settings;
    m_settings = settings;
    if (prev.Get<uint32_t>("msaa_count") != m_settings.Get<uint32_t>("msaa_count"))
    {
        CreateSizeDependentResources();
    }
}

void GeometryPass::CreateSizeDependentResources()
{
    output.position = m_device.CreateTexture(BindFlag::kRenderTarget | BindFlag::kShaderResource, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, m_settings.Get<uint32_t>("msaa_count"), m_width, m_height, 1);
    output.normal = m_device.CreateTexture(BindFlag::kRenderTarget | BindFlag::kShaderResource, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, m_settings.Get<uint32_t>("msaa_count"), m_width, m_height, 1);
    output.albedo = m_device.CreateTexture(BindFlag::kRenderTarget | BindFlag::kShaderResource, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, m_settings.Get<uint32_t>("msaa_count"), m_width, m_height, 1);
    output.material = m_device.CreateTexture(BindFlag::kRenderTarget | BindFlag::kShaderResource, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, m_settings.Get<uint32_t>("msaa_count"), m_width, m_height, 1);
    output.dsv = m_device.CreateTexture(BindFlag::kDepthStencil, gli::format::FORMAT_D24_UNORM_S8_UINT_PACK32, m_settings.Get<uint32_t>("msaa_count"), m_width, m_height, 1);
}

#include "GeometryPass.h"

#include <glm/gtx/transform.hpp>

GeometryPass::GeometryPass(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program(context)
{
    CreateSizeDependentResources();
    m_sampler = m_context.CreateSampler({
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

void GeometryPass::OnRender()
{
    m_context.SetViewport(m_width, m_height);

    m_context.UseProgram(m_program);

    m_program.ps.sampler.g_sampler.Attach(m_sampler);

    std::array<float, 4> color = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_program.ps.om.rtv0.Attach(output.position).Clear(color);
    m_program.ps.om.rtv1.Attach(output.normal).Clear(color);
    m_program.ps.om.rtv2.Attach(output.albedo).Clear(color);
    m_program.ps.om.rtv3.Attach(output.material).Clear(color);
    m_program.ps.om.dsv.Attach(output.dsv).Clear(ClearFlag::kDepth | ClearFlag::kStencil, 1.0f, 0);

    bool skiped = false;
    for (auto& model : m_input.scene_list)
    {
        if (!skiped && m_settings.skip_sponza_model)
        {
            skiped = true;
            continue;
        }
        m_program.vs.cbuffer.ConstantBuf.model = glm::transpose(model.matrix);
        m_program.vs.cbuffer.ConstantBuf.normalMatrix = glm::transpose(glm::transpose(glm::inverse(model.matrix)));
        m_program.ps.cbuffer.Settings.ibl_source = model.ibl_source;

        model.ia.indices.Bind();
        model.ia.positions.BindToSlot(m_program.vs.ia.POSITION);
        model.ia.normals.BindToSlot(m_program.vs.ia.NORMAL);
        model.ia.texcoords.BindToSlot(m_program.vs.ia.TEXCOORD);
        model.ia.tangents.BindToSlot(m_program.vs.ia.TANGENT);

        for (auto& range : model.ia.ranges)
        {
            auto& material = model.GetMaterial(range.id);

            m_program.ps.cbuffer.Settings.use_normal_mapping = material.texture.normal && m_settings.normal_mapping;
            m_program.ps.cbuffer.Settings.use_gloss_instead_of_roughness = material.texture.glossiness && !material.texture.roughness;
            m_program.ps.cbuffer.Settings.use_flip_normal_y = m_settings.use_flip_normal_y;

            m_program.ps.srv.normalMap.Attach(material.texture.normal);
            m_program.ps.srv.albedoMap.Attach(material.texture.albedo);
            m_program.ps.srv.glossMap.Attach(material.texture.glossiness);
            m_program.ps.srv.roughnessMap.Attach(material.texture.roughness);
            m_program.ps.srv.metalnessMap.Attach(material.texture.metalness);
            m_program.ps.srv.aoMap.Attach(material.texture.occlusion);
            m_program.ps.srv.alphaMap.Attach(material.texture.opacity);

            m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
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
    if (prev.msaa_count != settings.msaa_count)
    {
        CreateSizeDependentResources();
    }
}

void GeometryPass::CreateSizeDependentResources()
{
    output.position = m_context.CreateTexture(BindFlag::kRenderTarget | BindFlag::kShaderResource, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, m_settings.msaa_count, m_width, m_height, 1);
    output.normal = m_context.CreateTexture(BindFlag::kRenderTarget | BindFlag::kShaderResource, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, m_settings.msaa_count, m_width, m_height, 1);
    output.albedo = m_context.CreateTexture(BindFlag::kRenderTarget | BindFlag::kShaderResource, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, m_settings.msaa_count, m_width, m_height, 1);
    output.material = m_context.CreateTexture(BindFlag::kRenderTarget | BindFlag::kShaderResource, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, m_settings.msaa_count, m_width, m_height, 1);
    output.dsv = m_context.CreateTexture(BindFlag::kDepthStencil, gli::format::FORMAT_D24_UNORM_S8_UINT_PACK32, m_settings.msaa_count, m_width, m_height, 1);
}

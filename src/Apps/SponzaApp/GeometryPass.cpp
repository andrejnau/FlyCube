#include "GeometryPass.h"

#include <Utilities/DXUtility.h>
#include <Utilities/State.h>
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
    m_program.ps.cbuffer.Light.light_ambient = glm::vec3(m_settings.light_ambient);
    m_program.ps.cbuffer.Light.light_diffuse = glm::vec3(m_settings.light_diffuse);
    m_program.ps.cbuffer.Light.light_specular = glm::vec3(m_settings.light_specular);

    size_t cnt = 0;
    for (auto& model : m_input.scene_list)
        for (auto& cur_mesh : model.ia.ranges)
            ++cnt;
    m_program.SetMaxEvents(cnt);
}

void GeometryPass::OnRender()
{
    m_context.SetViewport(m_width, m_height);

    m_program.UseProgram();

    m_program.ps.sampler.g_sampler.Attach(m_sampler);

    float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_program.ps.om.rtv0.Attach(output.position).Clear(color);
    m_program.ps.om.rtv1.Attach(output.normal).Clear(color);
    m_program.ps.om.rtv2.Attach(output.ambient).Clear(color);
    m_program.ps.om.rtv3.Attach(output.diffuse).Clear(color);
    m_program.ps.om.rtv4.Attach(output.specular).Clear(color);
    m_program.ps.om.dsv.Attach(m_depth_stencil).Clear(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    for (auto& model : m_input.scene_list)
    {
        m_program.vs.cbuffer.ConstantBuf.model = glm::transpose(model.matrix);
        m_program.vs.cbuffer.ConstantBuf.normalMatrix = glm::transpose(glm::transpose(glm::inverse(model.matrix)));
        m_program.vs.cbuffer.ConstantBuf.normalMatrixView = glm::transpose(glm::transpose(glm::inverse(m_input.camera.GetViewMatrix() * model.matrix)));

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

            m_program.ps.cbuffer.Material.use_normal_mapping = !!material.texture.normal && !CurState::Instance().disable_norm;

            m_program.ps.srv.normalMap.Attach(material.texture.normal);
            m_program.ps.srv.alphaMap.Attach(material.texture.alpha);
            m_program.ps.srv.ambientMap.Attach(material.texture.diffuse);
            m_program.ps.srv.diffuseMap.Attach(material.texture.diffuse);
            m_program.ps.srv.specularMap.Attach(material.texture.specular);
            m_program.ps.srv.shininessMap.Attach(material.texture.shininess);

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

void GeometryPass::OnModifySettings(const Settings& settings)
{
    Settings prev = m_settings;
    m_settings = settings;
    if (prev.msaa_count != settings.msaa_count)
    {
        CreateSizeDependentResources();
    }
}

void GeometryPass::CreateSizeDependentResources()
{
    output.position = m_context.CreateTexture((BindFlag)(BindFlag::kRtv | BindFlag::kSrv), gli::format::FORMAT_RGBA32_SFLOAT_PACK32, m_settings.msaa_count, m_width, m_height, 1);
    output.normal = m_context.CreateTexture((BindFlag)(BindFlag::kRtv | BindFlag::kSrv), gli::format::FORMAT_RGBA32_SFLOAT_PACK32, m_settings.msaa_count, m_width, m_height, 1);
    output.ambient = m_context.CreateTexture((BindFlag)(BindFlag::kRtv | BindFlag::kSrv), gli::format::FORMAT_RGBA32_SFLOAT_PACK32, m_settings.msaa_count, m_width, m_height, 1);
    output.diffuse = m_context.CreateTexture((BindFlag)(BindFlag::kRtv | BindFlag::kSrv), gli::format::FORMAT_RGBA32_SFLOAT_PACK32, m_settings.msaa_count, m_width, m_height, 1);
    output.specular = m_context.CreateTexture((BindFlag)(BindFlag::kRtv | BindFlag::kSrv), gli::format::FORMAT_RGBA32_SFLOAT_PACK32, m_settings.msaa_count, m_width, m_height, 1);
    m_depth_stencil = m_context.CreateTexture((BindFlag)(BindFlag::kDsv), gli::format::FORMAT_D24_UNORM_S8_UINT_PACK32, m_settings.msaa_count, m_width, m_height, 1);
}

#include "GeometryPass.h"
#include "DX11CreateUtils.h"

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
    InitGBuffers();
}

void GeometryPass::OnUpdate()
{
    glm::mat4 projection, view, model;
    m_input.camera.GetMatrix(projection, view, model);
    
    m_program.vs.cbuffer.ConstantBuffer.view = glm::transpose(view);
    m_program.vs.cbuffer.ConstantBuffer.projection = glm::transpose(projection);
    m_program.ps.cbuffer.Light.light_ambient = glm::vec3(0.2f);
    m_program.ps.cbuffer.Light.light_diffuse = glm::vec3(1.0f);
    m_program.ps.cbuffer.Light.light_specular = glm::vec3(0.5f);
}

void GeometryPass::OnRender()
{
    m_program.UseProgram();
    m_context.device_context->IASetInputLayout(m_program.vs.input_layout.Get());

    std::vector<ID3D11RenderTargetView*> rtvs = {
        m_position_rtv.Get(),
        m_normal_rtv.Get(),
        m_ambient_rtv.Get(),
        m_diffuse_rtv.Get(),
        m_specular_rtv.Get(),
    };

    float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    for (auto & rtv : rtvs)
        m_context.device_context->ClearRenderTargetView(rtv, color);
    m_context.device_context->ClearDepthStencilView(m_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    m_context.device_context->OMSetRenderTargets(rtvs.size(), rtvs.data(), m_depth_stencil_view.Get());

    auto& state = CurState<bool>::Instance().state;
    for (auto& scene_item : m_input.scene_list)
    {
        m_program.vs.cbuffer.ConstantBuffer.model = glm::transpose(scene_item.matrix);
        m_program.vs.cbuffer.ConstantBuffer.normalMatrix = glm::transpose(glm::transpose(glm::inverse(scene_item.matrix)));
        m_program.vs.UpdateCBuffers();

        scene_item.model.bones.UpdateAnimation(glfwGetTime());

        ComPtr<ID3D11ShaderResourceView> bones_info_srv = CreateBufferSRV(m_context, scene_item.model.bones.bone_info);
        ComPtr<ID3D11ShaderResourceView> bones_srv = CreateBufferSRV(m_context, scene_item.model.bones.bone);

        m_program.vs.srv.bone_info.Attach(bones_info_srv);
        m_program.vs.srv.gBones.Attach(bones_srv);

        for (DX11Mesh& cur_mesh : scene_item.model.meshes)
        {
            cur_mesh.indices_buffer.Bind();
            cur_mesh.positions_buffer.BindToSlot(m_program.vs.geometry.POSITION);
            cur_mesh.normals_buffer.BindToSlot(m_program.vs.geometry.NORMAL);
            cur_mesh.texcoords_buffer.BindToSlot(m_program.vs.geometry.TEXCOORD);
            cur_mesh.tangents_buffer.BindToSlot(m_program.vs.geometry.TANGENT);
            cur_mesh.bones_offset_buffer.BindToSlot(m_program.vs.geometry.BONES_OFFSET);
            cur_mesh.bones_count_buffer.BindToSlot(m_program.vs.geometry.BONES_COUNT);

            if (!state["disable_norm"])
                m_program.ps.srv.normalMap.Attach(cur_mesh.GetTexture(aiTextureType_HEIGHT));
            else
                m_program.ps.srv.normalMap.Attach();

            m_program.ps.srv.alphaMap.Attach(cur_mesh.GetTexture(aiTextureType_OPACITY));
            m_program.ps.srv.ambientMap.Attach(cur_mesh.GetTexture(aiTextureType_AMBIENT));
            m_program.ps.srv.diffuseMap.Attach(cur_mesh.GetTexture(aiTextureType_DIFFUSE));
            m_program.ps.srv.specularMap.Attach(cur_mesh.GetTexture(aiTextureType_SPECULAR));
            m_program.ps.srv.glossMap.Attach(cur_mesh.GetTexture(aiTextureType_SHININESS));

            m_program.ps.cbuffer.Material.material_ambient = cur_mesh.material.amb;
            m_program.ps.cbuffer.Material.material_diffuse = cur_mesh.material.dif;
            m_program.ps.cbuffer.Material.material_specular = cur_mesh.material.spec;
            m_program.ps.cbuffer.Material.material_shininess = cur_mesh.material.shininess;

            m_program.ps.UpdateCBuffers();

            m_context.device_context->DrawIndexed(cur_mesh.indices.size(), 0, 0);
        }
    }
}

void GeometryPass::OnResize(int width, int height)
{
    if (width == m_width && height == m_height)
        return;

    m_width = width;
    m_height = height;
    InitGBuffers();
}

void GeometryPass::OnModifySettings(const Settings& settings)
{
    Settings prev = m_settings;
    m_settings = settings;
    if (prev.msaa_count != settings.msaa_count)
    {
        InitGBuffers();
    }
}

void GeometryPass::InitGBuffers()
{
    CreateRtvSrv(m_context, m_settings.msaa_count, m_width, m_height, m_position_rtv, output.position_srv);
    CreateRtvSrv(m_context, m_settings.msaa_count, m_width, m_height, m_normal_rtv, output.normal_srv);
    CreateRtvSrv(m_context, m_settings.msaa_count, m_width, m_height, m_ambient_rtv, output.ambient_srv);
    CreateRtvSrv(m_context, m_settings.msaa_count, m_width, m_height, m_diffuse_rtv, output.diffuse_srv);
    CreateRtvSrv(m_context, m_settings.msaa_count, m_width, m_height, m_specular_rtv, output.specular_srv);
    CreateDsv(m_context, m_settings.msaa_count, m_width, m_height, m_depth_stencil_view);
}

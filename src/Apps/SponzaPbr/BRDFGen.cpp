#include "BRDFGen.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

BRDFGen::BRDFGen(RenderDevice& device, const Input& input)
    : m_device(device)
    , m_input(input)
    , m_program(device)
{
    output.brdf = m_device.CreateTexture(BindFlag::kRenderTarget | BindFlag::kShaderResource, gli::format::FORMAT_RG32_SFLOAT_PACK32, 1, m_size, m_size, 1);
    m_dsv = m_device.CreateTexture(BindFlag::kDepthStencil, gli::format::FORMAT_D32_SFLOAT_PACK32, 1, m_size, m_size, 1);
}

void BRDFGen::OnUpdate()
{
}

void BRDFGen::OnRender(RenderCommandList& command_list)
{
    if (!is)
    {
        command_list.BeginEvent("DrawBRDF");
        DrawBRDF(command_list);
        command_list.EndEvent();

        is = true;
    }
}

void BRDFGen::DrawBRDF(RenderCommandList& command_list)
{
    command_list.SetViewport(0, 0, m_size, m_size);

    command_list.UseProgram(m_program);

    glm::vec4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
    RenderPassBeginDesc render_pass_desc = {};
    render_pass_desc.colors[m_program.ps.om.rtv0].texture = output.brdf;
    render_pass_desc.colors[m_program.ps.om.rtv0].clear_color = color;
    render_pass_desc.depth_stencil.texture = m_dsv;
    render_pass_desc.depth_stencil.clear_depth = 1.0f;

    m_input.square_model.ia.indices.Bind(command_list);
    m_input.square_model.ia.positions.BindToSlot(command_list, m_program.vs.ia.POSITION);
    m_input.square_model.ia.texcoords.BindToSlot(command_list, m_program.vs.ia.TEXCOORD);

    command_list.BeginRenderPass(render_pass_desc);
    for (auto& range : m_input.square_model.ia.ranges)
    {
        command_list.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
    }
    command_list.EndRenderPass();
}

void BRDFGen::OnResize(int width, int height)
{
}

void BRDFGen::OnModifySponzaSettings(const SponzaSettings& settings)
{
    m_settings = settings;
}

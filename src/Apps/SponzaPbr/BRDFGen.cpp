#include "BRDFGen.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

BRDFGen::BRDFGen(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_program(context)
{
    output.brdf = m_context.CreateTexture(BindFlag::kRenderTarget | BindFlag::kShaderResource, gli::format::FORMAT_RG32_SFLOAT_PACK32, 1, m_size, m_size, 1);
    m_dsv = m_context.CreateTexture(BindFlag::kDepthStencil, gli::format::FORMAT_D32_SFLOAT_PACK32, 1, m_size, m_size, 1);
}

void BRDFGen::OnUpdate()
{
}

void BRDFGen::OnRender(CommandListBox& command_list)
{
    if (!is)
    {
        command_list.BeginEvent("DrawBRDF");
        DrawBRDF(command_list);
        command_list.EndEvent();

        is = true;
    }
}

void BRDFGen::DrawBRDF(CommandListBox& command_list)
{
    command_list.SetViewport(m_size, m_size);

    command_list.UseProgram(m_program);

    std::array<float, 4> color = { 0.0f, 0.0f, 0.0f, 1.0f };
    command_list.Attach(m_program.ps.om.rtv0, output.brdf);
    command_list.ClearColor(m_program.ps.om.rtv0, color);
    command_list.Attach(m_program.ps.om.dsv, m_dsv);
    command_list.ClearDepth(m_program.ps.om.dsv, 1.0f);

    m_input.square_model.ia.indices.Bind();
    m_input.square_model.ia.positions.BindToSlot(m_program.vs.ia.POSITION);
    m_input.square_model.ia.texcoords.BindToSlot(m_program.vs.ia.TEXCOORD);

    for (auto& range : m_input.square_model.ia.ranges)
    {
        command_list.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
    }
}

void BRDFGen::OnResize(int width, int height)
{
}

void BRDFGen::OnModifySponzaSettings(const SponzaSettings& settings)
{
    m_settings = settings;
}

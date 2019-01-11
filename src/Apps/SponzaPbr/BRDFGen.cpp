#include "BRDFGen.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <Utilities/State.h>

BRDFGen::BRDFGen(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_program(context)
{
    output.brdf = m_context.CreateTexture((BindFlag)(BindFlag::kRtv | BindFlag::kSrv), gli::format::FORMAT_RG32_SFLOAT_PACK32, 1, m_size, m_size, 1);
    m_dsv = m_context.CreateTexture((BindFlag)(BindFlag::kDsv), gli::format::FORMAT_D32_SFLOAT_PACK32, 1, m_size, m_size, 1);
}

void BRDFGen::OnUpdate()
{
}

void BRDFGen::OnRender()
{
    if (!is)
    {
        m_context.BeginEvent("DrawBRDF");
        DrawBRDF();
        m_context.EndEvent();

        is = true;
    }
}

void BRDFGen::DrawBRDF()
{
    m_context.SetViewport(m_size, m_size);

    m_program.UseProgram();

    float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_program.ps.om.rtv0.Attach(output.brdf).Clear(color);
    m_program.ps.om.dsv.Attach(m_dsv).Clear(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_input.square_model.ia.indices.Bind();
    m_input.square_model.ia.positions.BindToSlot(m_program.vs.ia.POSITION);
    m_input.square_model.ia.texcoords.BindToSlot(m_program.vs.ia.TEXCOORD);

    for (auto& range : m_input.square_model.ia.ranges)
    {
        m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
    }
}

void BRDFGen::OnResize(int width, int height)
{
}

void BRDFGen::OnModifySettings(const Settings& settings)
{
    m_settings = settings;
}

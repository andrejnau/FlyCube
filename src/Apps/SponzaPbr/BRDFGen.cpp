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

void BRDFGen::OnRender()
{
    if (!is)
    {
        m_context->BeginEvent("DrawBRDF");
        DrawBRDF();
        m_context->EndEvent();

        is = true;
    }
}

void BRDFGen::DrawBRDF()
{
    m_context->SetViewport(m_size, m_size);

    m_context->UseProgram(m_program);

    std::array<float, 4> color = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_context->Attach(m_program.ps.om.rtv0, output.brdf);
    m_context->ClearColor(m_program.ps.om.rtv0, color);
    m_context->Attach(m_program.ps.om.dsv, m_dsv);
    m_context->ClearDepth(m_program.ps.om.dsv, 1.0f);

    m_input.square_model.ia.indices.Bind();
    m_input.square_model.ia.positions.BindToSlot(m_program.vs.ia.POSITION);
    m_input.square_model.ia.texcoords.BindToSlot(m_program.vs.ia.TEXCOORD);

    for (auto& range : m_input.square_model.ia.ranges)
    {
        m_context->DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
    }
}

void BRDFGen::OnResize(int width, int height)
{
}

void BRDFGen::OnModifySponzaSettings(const SponzaSettings& settings)
{
    m_settings = settings;
}

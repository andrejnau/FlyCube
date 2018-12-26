#include "BackgroundPass.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <Utilities/State.h>

BackgroundPass::BackgroundPass(Context& context, const Input& input, int width, int height)
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

void BackgroundPass::OnUpdate()
{
    m_program.vs.cbuffer.ConstantBuf.projection = glm::transpose(m_input.camera.GetProjectionMatrix());
    m_program.vs.cbuffer.ConstantBuf.view = glm::transpose(m_input.camera.GetViewMatrix());
}

void BackgroundPass::OnRender()
{
    m_context.SetViewport(m_width, m_height);

    m_program.UseProgram();

    m_program.SetDepthStencilState({ true, DepthComparison::kLessEqual });

    m_program.ps.sampler.g_sampler.Attach(m_sampler);

    m_program.ps.om.rtv0.Attach(m_input.rtv);
    m_program.ps.om.dsv.Attach(m_input.dsv);

    m_input.model.ia.indices.Bind();
    m_input.model.ia.positions.BindToSlot(m_program.vs.ia.POSITION);

    m_program.ps.srv.environmentMap.Attach(m_input.environment);
    for (auto& range : m_input.model.ia.ranges)
    {
        m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
    }
}

void BackgroundPass::OnResize(int width, int height)
{
    m_width = width;
    m_height = height;
    CreateSizeDependentResources();
}

void BackgroundPass::CreateSizeDependentResources()
{
}

void BackgroundPass::OnModifySettings(const Settings& settings)
{
    m_settings = settings;
}

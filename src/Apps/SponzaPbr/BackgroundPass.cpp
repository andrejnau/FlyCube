#include "BackgroundPass.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

BackgroundPass::BackgroundPass(RenderDevice& device, const Input& input, int width, int height)
    : m_device(device)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program(device)
{
    m_sampler = m_device.CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever }
    );
}

void BackgroundPass::OnUpdate()
{
    m_program.vs.cbuffer.ConstantBuf.projection = glm::transpose(m_input.camera.GetProjectionMatrix());
    m_program.vs.cbuffer.ConstantBuf.view = glm::transpose(m_input.camera.GetViewMatrix());
    m_program.vs.cbuffer.ConstantBuf.face = 0;
}

void BackgroundPass::OnRender(RenderCommandList& command_list)
{
    command_list.SetViewport(0, 0, m_width, m_height);

    command_list.UseProgram(m_program);
    command_list.Attach(m_program.vs.cbv.ConstantBuf, m_program.vs.cbuffer.ConstantBuf);

    command_list.SetDepthStencilState({ true, DepthComparison::kLessEqual });

    command_list.Attach(m_program.ps.sampler.g_sampler, m_sampler);

    RenderPassBeginDesc render_pass_desc = {};
    render_pass_desc.colors[m_program.ps.om.rtv0].texture = m_input.rtv;
    render_pass_desc.colors[m_program.ps.om.rtv0].load_op = RenderPassLoadOp::kLoad;
    render_pass_desc.depth_stencil.texture = m_input.dsv;
    render_pass_desc.depth_stencil.depth_load_op = RenderPassLoadOp::kLoad;

    m_input.model.ia.indices.Bind(command_list);
    m_input.model.ia.positions.BindToSlot(command_list, m_program.vs.ia.POSITION);

    command_list.Attach(m_program.ps.srv.environmentMap, m_input.environment);

    command_list.BeginRenderPass(render_pass_desc);
    for (auto& range : m_input.model.ia.ranges)
    {
        command_list.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
    }
    command_list.EndRenderPass();
}

void BackgroundPass::OnResize(int width, int height)
{
    m_width = width;
    m_height = height;
}

void BackgroundPass::OnModifySponzaSettings(const SponzaSettings& settings)
{
    m_settings = settings;
}

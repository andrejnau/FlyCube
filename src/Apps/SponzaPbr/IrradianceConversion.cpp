#include "IrradianceConversion.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

IrradianceConversion::IrradianceConversion(Device& device, const Input& input)
    : m_device(device)
    , m_input(input)
    , m_program_irradiance_convolution(device)
    , m_program_prefilter(device)
{
    m_sampler = m_device.CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever
    });
}

void IrradianceConversion::OnUpdate()
{
    m_program_irradiance_convolution.vs.cbuffer.ConstantBuf.projection = glm::transpose(glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f));
    m_program_prefilter.vs.cbuffer.ConstantBuf.projection = glm::transpose(glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f));
}

void IrradianceConversion::OnRender(RenderCommandList& command_list)
{
    if (!is || m_settings.Get<bool>("irradiance_conversion_every_frame"))
    {
        command_list.BeginEvent("DrawIrradianceConvolution");
        DrawIrradianceConvolution(command_list);
        command_list.EndEvent();

        command_list.BeginEvent("DrawPrefilter");
        DrawPrefilter(command_list);
        command_list.EndEvent();

        is = true;
    }
}

void IrradianceConversion::DrawIrradianceConvolution(RenderCommandList& command_list)
{
    command_list.SetViewport(0, 0, m_input.irradince.size, m_input.irradince.size);

    command_list.UseProgram(m_program_irradiance_convolution);
    command_list.Attach(m_program_irradiance_convolution.vs.cbv.ConstantBuf, m_program_irradiance_convolution.vs.cbuffer.ConstantBuf);

    command_list.Attach(m_program_irradiance_convolution.ps.sampler.g_sampler, m_sampler);

    glm::vec4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
    RenderPassBeginDesc render_pass_desc = {};
    render_pass_desc.colors[m_program_irradiance_convolution.ps.om.rtv0].texture = m_input.irradince.res;
    render_pass_desc.colors[m_program_irradiance_convolution.ps.om.rtv0].load_op = RenderPassLoadOp::kLoad;
    render_pass_desc.depth_stencil.texture = m_input.irradince.dsv;
    render_pass_desc.depth_stencil.clear_depth = 1.0f;

    m_input.model.ia.indices.Bind(command_list);
    m_input.model.ia.positions.BindToSlot(command_list, m_program_irradiance_convolution.vs.ia.POSITION);

    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Down = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 Left = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 ForwardRH = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 ForwardLH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardRH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardLH = glm::vec3(0.0f, 0.0f, -1.0f);

    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::mat4 capture_views[] =
    {
        glm::lookAt(position, position + Right, Up),
        glm::lookAt(position, position + Left, Up),
        glm::lookAt(position, position + Up, BackwardRH),
        glm::lookAt(position, position + Down, ForwardRH),
        glm::lookAt(position, position + BackwardLH, Up),
        glm::lookAt(position, position + ForwardLH, Up)
    };

    command_list.BeginRenderPass(render_pass_desc);
    for (uint32_t i = 0; i < 6; ++i)
    {
        m_program_irradiance_convolution.vs.cbuffer.ConstantBuf.face = 6 * m_input.irradince.layer + i;
        m_program_irradiance_convolution.vs.cbuffer.ConstantBuf.view = glm::transpose(capture_views[i]);
        command_list.Attach(m_program_irradiance_convolution.ps.srv.environmentMap, m_input.environment);
        for (auto& range : m_input.model.ia.ranges)
        {
            command_list.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }
    }
    command_list.EndRenderPass();
}

void IrradianceConversion::DrawPrefilter(RenderCommandList& command_list)
{
    command_list.UseProgram(m_program_prefilter);
    command_list.Attach(m_program_prefilter.ps.cbv.Settings, m_program_prefilter.ps.cbuffer.Settings);
    command_list.Attach(m_program_prefilter.vs.cbv.ConstantBuf, m_program_prefilter.vs.cbuffer.ConstantBuf);

    command_list.Attach(m_program_prefilter.ps.sampler.g_sampler, m_sampler);

    m_input.model.ia.indices.Bind(command_list);
    m_input.model.ia.positions.BindToSlot(command_list, m_program_prefilter.vs.ia.POSITION);

    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Down = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 Left = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 ForwardRH = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 ForwardLH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardRH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardLH = glm::vec3(0.0f, 0.0f, -1.0f);

    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::mat4 capture_views[] =
    {
        glm::lookAt(position, position + Right, Up),
        glm::lookAt(position, position + Left, Up),
        glm::lookAt(position, position + Up, BackwardRH),
        glm::lookAt(position, position + Down, ForwardRH),
        glm::lookAt(position, position + BackwardLH, Up),
        glm::lookAt(position, position + ForwardLH, Up)
    };

    size_t max_mip_levels = log2(m_input.prefilter.size);
    for (size_t mip = 0; mip < max_mip_levels; ++mip)
    {
        command_list.BeginEvent(std::string("DrawPrefilter: mip " + std::to_string(mip)).c_str());
        command_list.SetViewport(0, 0, m_input.prefilter.size >> mip, m_input.prefilter.size >> mip);
        m_program_prefilter.ps.cbuffer.Settings.roughness = (float)mip / (float)(max_mip_levels - 1);
        m_program_prefilter.ps.cbuffer.Settings.resolution = m_input.prefilter.size;

        glm::vec4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
        RenderPassBeginDesc render_pass_desc = {};
        render_pass_desc.colors[m_program_prefilter.ps.om.rtv0].texture = m_input.prefilter.res;
        render_pass_desc.colors[m_program_prefilter.ps.om.rtv0].load_op = RenderPassLoadOp::kLoad;
        render_pass_desc.colors[m_program_prefilter.ps.om.rtv0].view_desc = { mip };
        render_pass_desc.depth_stencil.texture = m_input.prefilter.dsv;
        render_pass_desc.depth_stencil.view_desc = { mip };
        render_pass_desc.depth_stencil.clear_depth = 1.0f;

        command_list.BeginRenderPass(render_pass_desc);
        for (uint32_t i = 0; i < 6; ++i)
        {
            command_list.BeginEvent(std::string("DrawPrefilter: mip " + std::to_string(mip) + " level " + std::to_string(i)).c_str());
            m_program_prefilter.vs.cbuffer.ConstantBuf.face = 6 * m_input.prefilter.layer + i;
            m_program_prefilter.vs.cbuffer.ConstantBuf.view = glm::transpose(capture_views[i]);
            command_list.Attach(m_program_prefilter.ps.srv.environmentMap, m_input.environment);
            for (auto& range : m_input.model.ia.ranges)
            {
                command_list.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
            }
            command_list.EndEvent();
        }
        command_list.EndRenderPass();
        command_list.EndEvent();
    }
}

void IrradianceConversion::OnModifySponzaSettings(const SponzaSettings& settings)
{
    m_settings = settings;
}

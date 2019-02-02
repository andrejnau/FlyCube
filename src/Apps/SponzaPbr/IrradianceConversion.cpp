#include "IrradianceConversion.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <Utilities/State.h>

IrradianceConversion::IrradianceConversion(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program_irradiance_convolution(context)
    , m_program_prefilter(context)
{
    CreateSizeDependentResources();

    m_sampler = m_context.CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever });
}

void IrradianceConversion::OnUpdate()
{
    m_program_irradiance_convolution.vs.cbuffer.ConstantBuf.projection = glm::transpose(glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f));
    m_program_prefilter.vs.cbuffer.ConstantBuf.projection = glm::transpose(glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f));
}

void IrradianceConversion::OnRender()
{
    if (!is || m_settings.irradiance_conversion_every_frame)
    {
        m_context.BeginEvent("DrawIrradianceConvolution");
        DrawIrradianceConvolution();
        m_context.EndEvent();
     
        m_context.BeginEvent("DrawPrefilter");
        DrawPrefilter();
        m_context.EndEvent();

        is = true;
    }
}

void IrradianceConversion::DrawIrradianceConvolution()
{
    m_context.SetViewport(m_input.irradince.size, m_input.irradince.size);

    m_program_irradiance_convolution.UseProgram();

    m_program_irradiance_convolution.ps.sampler.g_sampler.Attach(m_sampler);

    std::array<float, 4> color = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_program_irradiance_convolution.ps.om.rtv0.Attach(m_input.irradince.res);
    m_program_irradiance_convolution.ps.om.dsv.Attach(m_input.irradince.dsv).Clear(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_input.model.ia.indices.Bind();
    m_input.model.ia.positions.BindToSlot(m_program_irradiance_convolution.vs.ia.POSITION);

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

    for (uint32_t i = 0; i < 6; ++i)
    {
        m_program_irradiance_convolution.vs.cbuffer.ConstantBuf.face = 6 * m_input.irradince.layer + i;
        m_program_irradiance_convolution.vs.cbuffer.ConstantBuf.view = glm::transpose(capture_views[i]);
        m_program_irradiance_convolution.ps.srv.environmentMap.Attach(m_input.environment);
        for (auto& range : m_input.model.ia.ranges)
        {
            m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }
    }
}

void IrradianceConversion::DrawPrefilter()
{
    m_program_prefilter.UseProgram();

    m_program_prefilter.ps.sampler.g_sampler.Attach(m_sampler);

    m_input.model.ia.indices.Bind();
    m_input.model.ia.positions.BindToSlot(m_program_prefilter.vs.ia.POSITION);

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
        m_context.BeginEvent(std::string("DrawPrefilter: mip " + std::to_string(mip)).c_str());
        m_context.SetViewport(m_input.prefilter.size >> mip, m_input.prefilter.size >> mip);
        m_program_prefilter.ps.cbuffer.Settings.roughness = (float)mip / (float)(max_mip_levels - 1);
        m_program_prefilter.ps.cbuffer.Settings.resolution = m_input.prefilter.size;
        std::array<float, 4> color = { 0.0f, 0.0f, 0.0f, 1.0f };
        m_program_prefilter.ps.om.rtv0.Attach(m_input.prefilter.res, mip);
        m_program_prefilter.ps.om.dsv.Attach(m_input.prefilter.dsv, mip).Clear(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        for (uint32_t i = 0; i < 6; ++i)
        {
            m_context.BeginEvent(std::string("DrawPrefilter: mip " + std::to_string(mip) + " level " + std::to_string(i)).c_str());
            m_program_prefilter.vs.cbuffer.ConstantBuf.face = 6 * m_input.prefilter.layer + i;
            m_program_prefilter.vs.cbuffer.ConstantBuf.view = glm::transpose(capture_views[i]);
            m_program_prefilter.ps.srv.environmentMap.Attach(m_input.environment);
            for (auto& range : m_input.model.ia.ranges)
            {
                m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
            }
            m_context.EndEvent();
        }
        m_context.EndEvent();
    }
}

void IrradianceConversion::OnResize(int width, int height)
{
    m_width = width;
    m_height = height;
    CreateSizeDependentResources();
}

void IrradianceConversion::CreateSizeDependentResources()
{
}

void IrradianceConversion::OnModifySettings(const Settings& settings)
{
    m_settings = settings;
}

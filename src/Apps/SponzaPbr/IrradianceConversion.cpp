#include "IrradianceConversion.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <Utilities/State.h>

IrradianceConversion::IrradianceConversion(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program_equirectangular2cubemap(context)
    , m_program_irradiance_convolution(context)
{
    CreateSizeDependentResources();

    m_sampler = m_context.CreateSampler({
    SamplerFilter::kAnisotropic,
    SamplerTextureAddressMode::kWrap,
    SamplerComparisonFunc::kNever });
}

void IrradianceConversion::OnUpdate()
{
    m_program_equirectangular2cubemap.vs.cbuffer.ConstantBuf.projection = glm::transpose(glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f));
    m_program_equirectangular2cubemap.SetMaxEvents(6 * m_input.model.meshes.size());

    m_program_irradiance_convolution.vs.cbuffer.ConstantBuf.projection = glm::transpose(glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f));
    m_program_irradiance_convolution.SetMaxEvents(6 * m_input.model.meshes.size());
}

void IrradianceConversion::OnRender()
{
    static bool is = false;
    if (!is || m_settings.irradiance_conversion_every_frame)
    {
        DrawEquirectangular2Cubemap();
        if (m_input.is_ref)
            DrawIrradianceConvolution();
        else
            output.irradince = output.environment;
        is = true;
    }
}

void IrradianceConversion::DrawEquirectangular2Cubemap()
{
    m_context.SetViewport(m_texture_size, m_texture_size);

    m_program_equirectangular2cubemap.UseProgram();

    m_program_equirectangular2cubemap.ps.sampler.g_sampler.Attach(m_sampler);

    float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_program_equirectangular2cubemap.ps.om.rtv0.Attach(output.environment).Clear(color);
    decltype(auto) depth_out = m_program_equirectangular2cubemap.ps.om.dsv.Attach(m_depth_stencil_view);

    m_input.model.ia.indices.Bind();
    m_input.model.ia.positions.BindToSlot(m_program_equirectangular2cubemap.vs.ia.POSITION);

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
        depth_out.Clear(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        m_program_equirectangular2cubemap.vs.cbuffer.ConstantBuf.face = i;
        m_program_equirectangular2cubemap.vs.cbuffer.ConstantBuf.view = glm::transpose(capture_views[i]);
        m_program_equirectangular2cubemap.ps.srv.equirectangularMap.Attach(m_input.hdr);
        for (auto& range : m_input.model.ia.ranges)
        {
            m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }
    }
}

void IrradianceConversion::DrawIrradianceConvolution()
{
    m_context.SetViewport(m_irradince_texture_size, m_irradince_texture_size);

    m_program_irradiance_convolution.UseProgram();

    m_program_irradiance_convolution.ps.sampler.g_sampler.Attach(m_sampler);

    float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_program_irradiance_convolution.ps.om.rtv0.Attach(output.irradince).Clear(color);
    decltype(auto) depth_out = m_program_irradiance_convolution.ps.om.dsv.Attach(m_depth_stencil_view);

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
        depth_out.Clear(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        m_program_irradiance_convolution.vs.cbuffer.ConstantBuf.face = i;
        m_program_irradiance_convolution.vs.cbuffer.ConstantBuf.view = glm::transpose(capture_views[i]);
        m_program_irradiance_convolution.ps.srv.environmentMap.Attach(output.environment);
        for (auto& range : m_input.model.ia.ranges)
        {
            m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }
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
    output.environment = m_context.CreateTexture((BindFlag)(BindFlag::kRtv | BindFlag::kSrv), gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, m_texture_size, m_texture_size, 6);
    output.irradince = m_context.CreateTexture((BindFlag)(BindFlag::kRtv | BindFlag::kSrv), gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, m_irradince_texture_size, m_irradince_texture_size, 6);
    m_depth_stencil_view = m_context.CreateTexture((BindFlag)(BindFlag::kDsv), gli::format::FORMAT_D32_SFLOAT_PACK32, 1, m_texture_size, m_texture_size, 1);
}

void IrradianceConversion::OnModifySettings(const Settings& settings)
{
    m_settings = settings;
}

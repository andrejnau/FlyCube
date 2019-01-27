#include "Equirectangular2Cubemap.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <Utilities/State.h>

Equirectangular2Cubemap::Equirectangular2Cubemap(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program_equirectangular2cubemap(context)
    , m_program_downsample(context)
{
    CreateSizeDependentResources();

    m_sampler = m_context.CreateSampler({
    SamplerFilter::kAnisotropic,
    SamplerTextureAddressMode::kWrap,
    SamplerComparisonFunc::kNever });
}

void Equirectangular2Cubemap::OnUpdate()
{
    m_program_equirectangular2cubemap.vs.cbuffer.ConstantBuf.projection = glm::transpose(glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f));
}

void Equirectangular2Cubemap::OnRender()
{
    if (!is || m_settings.irradiance_conversion_every_frame)
    {
        m_context.BeginEvent("DrawEquirectangular2Cubemap");
        DrawEquirectangular2Cubemap();
        m_context.EndEvent();

        is = true;
    }
}

void Equirectangular2Cubemap::DrawEquirectangular2Cubemap()
{
    m_context.SetViewport(m_texture_size, m_texture_size);

    m_program_equirectangular2cubemap.UseProgram();

    m_program_equirectangular2cubemap.ps.sampler.g_sampler.Attach(m_sampler);

    float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_program_equirectangular2cubemap.ps.om.rtv0.Attach(output.environment).Clear(color);
    m_program_equirectangular2cubemap.ps.om.dsv.Attach(m_dsv).Clear(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

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
        m_program_equirectangular2cubemap.vs.cbuffer.ConstantBuf.face = i;
        m_program_equirectangular2cubemap.vs.cbuffer.ConstantBuf.view = glm::transpose(capture_views[i]);
        m_program_equirectangular2cubemap.ps.srv.equirectangularMap.Attach(m_input.hdr);
        for (auto& range : m_input.model.ia.ranges)
        {
            m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }
    }

    m_program_downsample.UseProgram();
    for (size_t i = 1; i < m_texture_mips; ++i)
    {
        m_program_downsample.cs.srv.inputTexture.Attach(output.environment, {i - 1, 1});
        m_program_downsample.cs.uav.outputTexture.Attach(output.environment, i);
        m_context.Dispatch((m_texture_size >> i) / 8, (m_texture_size >> i) / 8, 6);
    }
}

void Equirectangular2Cubemap::OnResize(int width, int height)
{
    m_width = width;
    m_height = height;
    CreateSizeDependentResources();
}

void Equirectangular2Cubemap::CreateSizeDependentResources()
{
    m_texture_mips = 0;
    for (size_t i = 0; ; ++i)
    {
        if ((m_texture_size >> i) % 8 != 0)
            break;
        ++m_texture_mips;
    }

    output.environment = m_context.CreateTexture((BindFlag)(BindFlag::kRtv | BindFlag::kSrv | BindFlag::kUav), gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, m_texture_size, m_texture_size, 6, m_texture_mips);
    m_dsv = m_context.CreateTexture((BindFlag)(BindFlag::kDsv), gli::format::FORMAT_D32_SFLOAT_PACK32, 1, m_texture_size, m_texture_size, 6);
}

void Equirectangular2Cubemap::OnModifySettings(const Settings& settings)
{
    m_settings = settings;
}

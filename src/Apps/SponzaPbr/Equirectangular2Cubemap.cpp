#include "Equirectangular2Cubemap.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

Equirectangular2Cubemap::Equirectangular2Cubemap(Device& device, const Input& input, int width, int height)
    : m_device(device)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program_equirectangular2cubemap(device)
    , m_program_downsample(device)
{
    CreateSizeDependentResources();

    m_sampler = m_device.CreateSampler({
    SamplerFilter::kAnisotropic,
    SamplerTextureAddressMode::kWrap,
    SamplerComparisonFunc::kNever });
}

void Equirectangular2Cubemap::OnUpdate()
{
    m_program_equirectangular2cubemap.vs.cbuffer.ConstantBuf.projection = glm::transpose(glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f));
}

void Equirectangular2Cubemap::OnRender(CommandListBox& command_list)
{
    if (!is || m_settings.Get<bool>("irradiance_conversion_every_frame"))
    {
        command_list.BeginEvent("DrawEquirectangular2Cubemap");
        DrawEquirectangular2Cubemap(command_list);
        command_list.EndEvent();

        is = true;
    }
}

void Equirectangular2Cubemap::DrawEquirectangular2Cubemap(CommandListBox& command_list)
{
    command_list.SetViewport(m_texture_size, m_texture_size);

    command_list.UseProgram(m_program_equirectangular2cubemap);
    command_list.Attach(m_program_equirectangular2cubemap.vs.cbv.ConstantBuf, m_program_equirectangular2cubemap.vs.cbuffer.ConstantBuf);

    command_list.Attach(m_program_equirectangular2cubemap.ps.sampler.g_sampler, m_sampler);

    std::array<float, 4> color = { 0.0f, 0.0f, 0.0f, 1.0f };
    command_list.Attach(m_program_equirectangular2cubemap.ps.om.rtv0, output.environment);
    command_list.ClearColor(m_program_equirectangular2cubemap.ps.om.rtv0, color);
    command_list.Attach(m_program_equirectangular2cubemap.ps.om.dsv, m_dsv);
    command_list.ClearDepth(m_program_equirectangular2cubemap.ps.om.dsv, 1.0f);

    m_input.model.ia.indices.Bind(command_list);
    m_input.model.ia.positions.BindToSlot(command_list, m_program_equirectangular2cubemap.vs.ia.POSITION);

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
        command_list.Attach(m_program_equirectangular2cubemap.ps.srv.equirectangularMap, m_input.hdr);
        for (auto& range : m_input.model.ia.ranges)
        {
            command_list.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }
    }

    command_list.UseProgram(m_program_downsample);
    for (size_t i = 1; i < m_texture_mips; ++i)
    {
        command_list.Attach(m_program_downsample.cs.srv.inputTexture, output.environment, {i - 1, 1});
        command_list.Attach(m_program_downsample.cs.uav.outputTexture, output.environment, {i, 1});
        command_list.Dispatch((m_texture_size >> i) / 8, (m_texture_size >> i) / 8, 6);
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

    output.environment = m_device.CreateTexture(BindFlag::kRenderTarget | BindFlag::kShaderResource | BindFlag::kUnorderedAccess, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, m_texture_size, m_texture_size, 6, m_texture_mips);
    m_dsv = m_device.CreateTexture(BindFlag::kDepthStencil, gli::format::FORMAT_D32_SFLOAT_PACK32, 1, m_texture_size, m_texture_size, 6);
}

void Equirectangular2Cubemap::OnModifySponzaSettings(const SponzaSettings& settings)
{
    m_settings = settings;
}

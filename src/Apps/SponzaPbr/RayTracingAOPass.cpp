#include "RayTracingAOPass.h"

RayTracingAOPass::RayTracingAOPass(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_raytracing_program(context)
    , m_program_blur(context)
{
    CreateSizeDependentResources();
}

void RayTracingAOPass::OnUpdate()
{
    m_raytracing_program.lib.cbuffer.Settings.ao_radius = m_settings.ao_radius;
    m_raytracing_program.lib.cbuffer.Settings.num_rays = m_settings.rtao_num_rays;

    if (!m_is_initialized)
    {
        m_raytracing_program.lib.cbuffer.Settings.frame_index = 0;

        std::vector<std::pair<Resource::Ptr, glm::mat4>> geometry;
        for (auto& model : m_input.scene_list)
        {
            for (auto& range : model.ia.ranges)
            {
                BufferDesc vertex = {
                        model.ia.positions.GetBuffer(),
                        gli::format::FORMAT_RGB32_SFLOAT_PACK32,
                        range.index_count,
                        range.base_vertex_location
                };

                BufferDesc index = {
                     model.ia.indices.GetBuffer(),
                     model.ia.indices.Format(),
                     range.index_count,
                     range.start_index_location
                };

                m_bottom.emplace_back(m_context.CreateBottomLevelAS(vertex, index));
                geometry.emplace_back(m_bottom.back(), glm::transpose(model.matrix));
            }
        }

        m_top = m_context.CreateTopLevelAS(geometry);

        m_is_initialized = true;
    }
    else
    {
        ++m_raytracing_program.lib.cbuffer.Settings.frame_index;
    }
}

void RayTracingAOPass::OnRender()
{
    if (!m_settings.use_rtao)
        return;

    m_raytracing_program.UseProgram();
    m_raytracing_program.lib.srv.gPosition.Attach(m_input.geometry_pass.position);
    m_raytracing_program.lib.srv.gNormal.Attach(m_input.geometry_pass.normal);
    m_raytracing_program.lib.srv.geometry.Attach(m_top);
    m_raytracing_program.lib.uav.result.Attach(m_ao);
    m_context.DispatchRays(m_width, m_height, 1);

    if (m_settings.use_ao_blur)
    {
        m_program_blur.UseProgram();
        m_program_blur.ps.uav.out_uav.Attach(m_ao_blur);

        m_input.square.ia.indices.Bind();
        m_input.square.ia.positions.BindToSlot(m_program_blur.vs.ia.POSITION);
        m_input.square.ia.texcoords.BindToSlot(m_program_blur.vs.ia.TEXCOORD);
        for (auto& range : m_input.square.ia.ranges)
        {
            m_program_blur.ps.srv.ssaoInput.Attach(m_ao);
            m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }

        output.ao = m_ao_blur;
    }
    else
    {
        output.ao = m_ao;
    }
}

void RayTracingAOPass::OnResize(int width, int height)
{
    m_width = width;
    m_height = height;
    CreateSizeDependentResources();
}

void RayTracingAOPass::CreateSizeDependentResources()
{
    m_ao = m_context.CreateTexture((BindFlag)(BindFlag::kRtv | BindFlag::kSrv | BindFlag::kUav), gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, m_width, m_height, 1);
    m_ao_blur = m_context.CreateTexture((BindFlag)(BindFlag::kRtv | BindFlag::kSrv | BindFlag::kUav), gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, m_width, m_height, 1);
}

void RayTracingAOPass::OnModifySettings(const Settings& settings)
{
    m_settings = settings;
}

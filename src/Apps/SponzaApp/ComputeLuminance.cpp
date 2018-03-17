#include "ComputeLuminance.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <Utilities/State.h>

ComputeLuminance::ComputeLuminance(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_HDRLum1DPassCS(context)
    , m_HDRLum2DPassCS(context)
    , m_HDRApply(context)
{
    CreateBuffers();
}

void ComputeLuminance::OnUpdate()
{
    m_HDRLum2DPassCS.SetMaxEvents(1);
    m_HDRLum1DPassCS.SetMaxEvents(2);
    int cnt = 0;
    for (auto& cur_mesh : m_input.model.ia.ranges)
        ++cnt;
    m_HDRApply.SetMaxEvents(cnt);
}

void ComputeLuminance::GetLum2DPassCS(size_t buf_id, uint32_t thread_group_x, uint32_t thread_group_y)
{
    m_HDRLum2DPassCS.cs.cbuffer.cbv.dispatchSize = glm::uvec2(thread_group_x, thread_group_y);
    m_HDRLum2DPassCS.UseProgram();

    m_HDRLum2DPassCS.cs.uav.result.Attach(m_use_res[buf_id]);
    m_HDRLum2DPassCS.cs.srv.input.Attach(m_input.hdr_res);
    m_context.Dispatch(thread_group_x, thread_group_y, 1);
}

void ComputeLuminance::GetLum1DPassCS(size_t buf_id, uint32_t input_buffer_size, uint32_t thread_group_x)
{
    m_HDRLum1DPassCS.cs.cbuffer.cbv.bufferSize = input_buffer_size;
    m_HDRLum1DPassCS.UseProgram();

    m_HDRLum1DPassCS.cs.srv.input.Attach(m_use_res[buf_id - 1]);
    m_HDRLum1DPassCS.cs.uav.result.Attach(m_use_res[buf_id]);

    m_context.Dispatch(thread_group_x, 1, 1);

    m_HDRLum1DPassCS.cs.uav.result.Attach();
}

void ComputeLuminance::Draw(size_t buf_id)
{
    m_HDRApply.ps.cbuffer.cbv.dim = glm::uvec2(m_width, m_height);
    m_HDRApply.ps.cbuffer.$Globals.Exposure = m_settings.Exposure;
    m_HDRApply.ps.cbuffer.$Globals.White = m_settings.White;

    if (!m_use_res.empty())
        m_HDRApply.ps.cbuffer.cbv.use_tone_mapping = m_settings.use_tone_mapping;
    else
        m_HDRApply.ps.cbuffer.cbv.use_tone_mapping = false;

    m_HDRApply.UseProgram();

    float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_HDRApply.ps.om.rtv0.Attach(m_input.rtv).Clear(color);
    m_HDRApply.ps.om.dsv.Attach(m_input.dsv).Clear(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_input.model.ia.indices.Bind();
    m_input.model.ia.positions.BindToSlot(m_HDRApply.vs.ia.POSITION);
    m_input.model.ia.texcoords.BindToSlot(m_HDRApply.vs.ia.TEXCOORD);

    for (auto& range : m_input.model.ia.ranges)
    {
        m_HDRApply.ps.srv.hdr_input.Attach(m_input.hdr_res);
        m_HDRApply.ps.srv.lum.Attach(m_use_res[buf_id]);
        m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
    }
}

void ComputeLuminance::OnRender()
{
    m_context.SetViewport(m_width, m_height);
    size_t buf_id = 0;
    if (m_settings.use_tone_mapping)
    {
        GetLum2DPassCS(buf_id, m_thread_group_x, m_thread_group_y);
        for (int block_size = m_thread_group_x * m_thread_group_y; block_size > 1;)
        {
            uint32_t next_block_size = (block_size + 127) / 128;
            GetLum1DPassCS(++buf_id, block_size, next_block_size);
            block_size = next_block_size;
        }
        Draw(buf_id);
    }
    else
    {
        Draw(buf_id);
    }
}

void ComputeLuminance::OnResize(int width, int height)
{
    CreateBuffers();
}

void ComputeLuminance::CreateBuffers()
{
    m_use_res.clear();
    m_thread_group_x = (m_width + 31) / 32;
    m_thread_group_y = (m_height + 31) / 32;
    uint32_t total_invoke = m_thread_group_x * m_thread_group_y;
    Resource::Ptr buffer = m_context.CreateBuffer(BindFlag::kUav | BindFlag::kSrv, sizeof(float) * total_invoke, 4);
    m_use_res.push_back(buffer);

    for (int block_size = m_thread_group_x * m_thread_group_y; block_size > 1;)
    {
        uint32_t next_block_size = (block_size + 127) / 128;
        Resource::Ptr buffer = m_context.CreateBuffer(BindFlag::kUav | BindFlag::kSrv, sizeof(float) * next_block_size, 4);
        m_use_res.push_back(buffer);
        block_size = next_block_size;
    }
}

void ComputeLuminance::OnModifySettings(const Settings& settings)
{
    m_settings = settings;
}

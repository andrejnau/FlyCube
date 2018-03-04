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
}

void ComputeLuminance::OnUpdate()
{
    m_HDRLum2DPassCS.SetMaxEvents(1);
    m_HDRLum1DPassCS.SetMaxEvents(1);
    int cnt = 0;
    for (Mesh& cur_mesh : m_input.model.meshes)
        ++cnt;
    m_HDRApply.SetMaxEvents(cnt);
}

Resource::Ptr ComputeLuminance::GetLum2DPassCS(uint32_t thread_group_x, uint32_t thread_group_y)
{
    m_HDRLum2DPassCS.cs.cbuffer.cbv.dispatchSize = glm::uvec2(thread_group_x, thread_group_y);
    m_HDRLum2DPassCS.UseProgram();

    uint32_t total_invoke = thread_group_x * thread_group_y;

    Resource::Ptr buffer = m_context.CreateBuffer(BindFlag::kUav | BindFlag::kSrv, sizeof(float) * total_invoke, 4);

    m_HDRLum2DPassCS.cs.uav.result.Attach(buffer);
    m_HDRLum2DPassCS.cs.srv.input.Attach(m_input.hdr_res);
    m_context.Dispatch(thread_group_x, thread_group_y, 1);

    return buffer;
}

Resource::Ptr ComputeLuminance::GetLum1DPassCS(Resource::Ptr input, uint32_t input_buffer_size, uint32_t thread_group_x)
{
    m_HDRLum1DPassCS.cs.cbuffer.cbv.bufferSize = input_buffer_size;
    m_HDRLum1DPassCS.UseProgram();

    Resource::Ptr buffer = m_context.CreateBuffer(BindFlag::kUav | BindFlag::kSrv, sizeof(float) * thread_group_x, 4);
 
    m_HDRLum1DPassCS.cs.uav.result.Attach(buffer);
    m_HDRLum1DPassCS.cs.srv.input.Attach(input);

    m_context.Dispatch(thread_group_x, 1, 1);

    m_HDRLum1DPassCS.cs.uav.result.Attach();

    return buffer;
}

void ComputeLuminance::Draw(Resource::Ptr input)
{
    m_HDRApply.ps.cbuffer.cbv.dim = glm::uvec2(m_width, m_height);
    m_HDRApply.ps.cbuffer.$Globals.Exposure = m_settings.Exposure;
    m_HDRApply.ps.cbuffer.$Globals.White = m_settings.White;

    if (input)
        m_HDRApply.ps.cbuffer.cbv.use_tone_mapping = m_settings.use_tone_mapping;
    else
        m_HDRApply.ps.cbuffer.cbv.use_tone_mapping = false;

    m_HDRApply.UseProgram();

    float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_context.OMSetRenderTargets({ m_input.rtv }, m_input.dsv);
    m_context.ClearRenderTarget(m_input.rtv, color);
    m_context.ClearDepthStencil(m_input.dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    for (Mesh& cur_mesh : m_input.model.meshes)
    {
        cur_mesh.indices_buffer.Bind();
        cur_mesh.positions_buffer.BindToSlot(m_HDRApply.vs.ia.POSITION);
        cur_mesh.texcoords_buffer.BindToSlot(m_HDRApply.vs.ia.TEXCOORD);

        m_HDRApply.ps.srv.hdr_input.Attach(m_input.hdr_res);
        m_HDRApply.ps.srv.lum.Attach(input);
        m_context.DrawIndexed(cur_mesh.indices.size());
    }
}

void ComputeLuminance::OnRender()
{
    m_context.SetViewport(m_width, m_height);

    if (m_settings.use_tone_mapping)
    {
        uint32_t thread_group_x = (m_width + 31) / 32;
        uint32_t thread_group_y = (m_height + 31) / 32;

        auto buf = GetLum2DPassCS(thread_group_x, thread_group_y);
        for (int block_size = thread_group_x * thread_group_y; block_size > 1;)
        {
            uint32_t next_block_size = (block_size + 127) / 128;
            buf = GetLum1DPassCS(buf, block_size, next_block_size);
            block_size = next_block_size;
        }
        Draw(buf);
    }
    else
    {
        Draw(nullptr);
    }
}

void ComputeLuminance::OnResize(int width, int height)
{
}

void ComputeLuminance::OnModifySettings(const Settings& settings)
{
    m_settings = settings;
}

#include "ComputeLuminance.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

ComputeLuminance::ComputeLuminance(RenderDevice& device, const Input& input, int width, int height)
    : m_device(device)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_HDRLum1DPassCS(device)
    , m_HDRLum2DPassCS(device)
    , m_HDRApply(device)
{
    CreateBuffers();
}

void ComputeLuminance::OnUpdate()
{
}

void ComputeLuminance::GetLum2DPassCS(RenderCommandList& command_list, size_t buf_id, uint32_t thread_group_x, uint32_t thread_group_y)
{
    m_HDRLum2DPassCS.cs.cbuffer.cb.dispatchSize = glm::uvec2(thread_group_x, thread_group_y);
    command_list.UseProgram(m_HDRLum2DPassCS);
    command_list.Attach(m_HDRLum2DPassCS.cs.cbv.cb, m_HDRLum2DPassCS.cs.cbuffer.cb);

    command_list.Attach(m_HDRLum2DPassCS.cs.uav.result, m_use_res[buf_id]);
    command_list.Attach(m_HDRLum2DPassCS.cs.srv.data, m_input.hdr_res);
    command_list.Dispatch(thread_group_x, thread_group_y, 1);
}

void ComputeLuminance::GetLum1DPassCS(RenderCommandList& command_list, size_t buf_id, uint32_t input_buffer_size, uint32_t thread_group_x)
{
    m_HDRLum1DPassCS.cs.cbuffer.cb.bufferSize = input_buffer_size;
    command_list.UseProgram(m_HDRLum1DPassCS);
    command_list.Attach(m_HDRLum1DPassCS.cs.cbv.cb, m_HDRLum1DPassCS.cs.cbuffer.cb);

    command_list.Attach(m_HDRLum1DPassCS.cs.srv.data, m_use_res[buf_id - 1]);
    command_list.Attach(m_HDRLum1DPassCS.cs.uav.result, m_use_res[buf_id]);

    command_list.Dispatch(thread_group_x, 1, 1);

    command_list.Attach(m_HDRLum1DPassCS.cs.uav.result);
}

void ComputeLuminance::Draw(RenderCommandList& command_list, size_t buf_id)
{
    m_HDRApply.ps.cbuffer.HDRSetting.gamma_correction = m_settings.Get<bool>("gamma_correction");
    m_HDRApply.ps.cbuffer.HDRSetting.use_reinhard_tone_operator = m_settings.Get<bool>("use_reinhard_tone_operator");
    m_HDRApply.ps.cbuffer.HDRSetting.use_tone_mapping = m_settings.Get<bool>("use_tone_mapping");
    m_HDRApply.ps.cbuffer.HDRSetting.use_white_balance = m_settings.Get<bool>("use_white_balance");
    m_HDRApply.ps.cbuffer.HDRSetting.use_filmic_hdr = m_settings.Get<bool>("use_filmic_hdr");
    m_HDRApply.ps.cbuffer.HDRSetting.use_avg_lum = m_settings.Get<bool>("use_avg_lum") && !m_use_res.empty();
    m_HDRApply.ps.cbuffer.HDRSetting.exposure = m_settings.Get<float>("exposure");
    m_HDRApply.ps.cbuffer.HDRSetting.white = m_settings.Get<float>("white");

    command_list.UseProgram(m_HDRApply);
    command_list.Attach(m_HDRApply.ps.cbv.HDRSetting, m_HDRApply.ps.cbuffer.HDRSetting);

    glm::vec4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
    RenderPassBeginDesc render_pass_desc = {};
    render_pass_desc.colors[m_HDRApply.ps.om.rtv0].texture = m_input.rtv;
    render_pass_desc.colors[m_HDRApply.ps.om.rtv0].clear_color = color;
    render_pass_desc.depth_stencil.texture = m_input.dsv;
    render_pass_desc.depth_stencil.clear_depth = 1.0f;

    m_input.model.ia.indices.Bind(command_list);
    m_input.model.ia.positions.BindToSlot(command_list, m_HDRApply.vs.ia.POSITION);
    m_input.model.ia.texcoords.BindToSlot(command_list, m_HDRApply.vs.ia.TEXCOORD);

    command_list.BeginRenderPass(render_pass_desc);
    for (auto& range : m_input.model.ia.ranges)
    {
        command_list.Attach(m_HDRApply.ps.srv.hdr_input, m_input.hdr_res);
        command_list.Attach(m_HDRApply.ps.srv.lum, m_use_res[buf_id]);
        command_list.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
    }
    command_list.EndRenderPass();
}

void ComputeLuminance::OnRender(RenderCommandList& command_list)
{
    command_list.SetViewport(0, 0, m_width, m_height);
    size_t buf_id = 0;
    if (m_settings.Get<bool>("use_tone_mapping"))
    {
        GetLum2DPassCS(command_list, buf_id, m_thread_group_x, m_thread_group_y);
        for (int block_size = m_thread_group_x * m_thread_group_y; block_size > 1;)
        {
            uint32_t next_block_size = (block_size + 127) / 128;
            GetLum1DPassCS(command_list, ++buf_id, block_size, next_block_size);
            block_size = next_block_size;
        }
        Draw(command_list, buf_id);
    }
    else
    {
        Draw(command_list, buf_id);
    }
}

void ComputeLuminance::OnResize(int width, int height)
{
    m_width = width;
    m_height = height;
    CreateBuffers();
}

void ComputeLuminance::CreateBuffers()
{
    m_use_res.clear();
    m_thread_group_x = (m_width + 31) / 32;
    m_thread_group_y = (m_height + 31) / 32;
    uint32_t total_invoke = m_thread_group_x * m_thread_group_y;
    std::shared_ptr<Resource> buffer = m_device.CreateBuffer(BindFlag::kUnorderedAccess | BindFlag::kShaderResource, sizeof(float) * total_invoke);
    m_use_res.push_back(buffer);

    for (int block_size = m_thread_group_x * m_thread_group_y; block_size > 1;)
    {
        uint32_t next_block_size = (block_size + 127) / 128;
        std::shared_ptr<Resource> buffer = m_device.CreateBuffer(BindFlag::kUnorderedAccess | BindFlag::kShaderResource, sizeof(float) * next_block_size);
        m_use_res.push_back(buffer);
        block_size = next_block_size;
    }
}

void ComputeLuminance::OnModifySponzaSettings(const SponzaSettings& settings)
{
    m_settings = settings;
}

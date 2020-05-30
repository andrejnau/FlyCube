#include "ProgramApi/ProgramApi.h"

ProgramApi::ProgramApi(Context& context)
    : m_context(context)
    , m_cbv_data(context.FrameCount)
{
}

std::shared_ptr<Program>& ProgramApi::GetProgram()
{
    return m_program;
}

Context& ProgramApi::GetContext()
{
    return m_context;
}

void ProgramApi::OnPresent()
{
    m_cbv_data[m_context.GetFrameIndex()].cbv_offset.clear();
}

void ProgramApi::SetCBufferLayout(const NamedBindKey& bind_key, BufferLayout& buffer_layout)
{
    m_cbv_layout.emplace(std::piecewise_construct,
        std::forward_as_tuple(bind_key),
        std::forward_as_tuple(buffer_layout));
}

void ProgramApi::UpdateCBuffers()
{
    for (auto& x : m_cbv_layout)
    {
        auto& cbv_buffer = m_cbv_data[m_context.GetFrameIndex()].cbv_buffer;
        auto& cbv_offset = m_cbv_data[m_context.GetFrameIndex()].cbv_offset;
        BufferLayout& buffer_layout = x.second;

        auto& buffer_data = buffer_layout.GetBuffer();
        bool change_buffer = buffer_layout.SyncData();
        change_buffer = change_buffer || !cbv_offset.count(x.first);
        if (change_buffer && cbv_offset.count(x.first))
            ++cbv_offset[x.first];
        if (cbv_offset[x.first] >= cbv_buffer[x.first].size())
            cbv_buffer[x.first].push_back(m_context.m_device->CreateBuffer(BindFlag::kConstantBuffer, static_cast<uint32_t>(buffer_data.size()), MemoryType::kUpload));

        auto& res = cbv_buffer[x.first][cbv_offset[x.first]];
        if (change_buffer)
        {
            m_context.UpdateSubresource(res, 0, buffer_layout.GetBuffer().data(), 0, 0);
        }

        m_context.Attach(x.first, res, {});
    }
}

#pragma once
#include <directx/d3d12.h>

#include <vector>

class DXStateBuilder {
public:
    template <typename T, typename U>
    void AddState(const U& state)
    {
        size_t offset = m_data.size();
        m_data.resize(offset + sizeof(T));
        reinterpret_cast<T&>(m_data[offset]) = state;
    }

    D3D12_PIPELINE_STATE_STREAM_DESC GetDesc()
    {
        D3D12_PIPELINE_STATE_STREAM_DESC stream_desc = {};
        stream_desc.pPipelineStateSubobjectStream = m_data.data();
        stream_desc.SizeInBytes = m_data.size();
        return stream_desc;
    }

private:
    std::vector<uint8_t> m_data;
};

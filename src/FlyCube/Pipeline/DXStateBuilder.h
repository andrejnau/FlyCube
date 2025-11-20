#pragma once
#if defined(_WIN32)
#include <wrl.h>
#else
#include <wsl/wrladapter.h>
#endif

#include <directx/d3d12.h>

#include <vector>

class DXStateBuilder {
public:
    template <typename T, typename U>
    void AddState(const U& state)
    {
        size_t offset = data_.size();
        data_.resize(offset + sizeof(T));
        reinterpret_cast<T&>(data_[offset]) = state;
    }

    D3D12_PIPELINE_STATE_STREAM_DESC GetDesc()
    {
        D3D12_PIPELINE_STATE_STREAM_DESC stream_desc = {};
        stream_desc.pPipelineStateSubobjectStream = data_.data();
        stream_desc.SizeInBytes = data_.size();
        return stream_desc;
    }

private:
    std::vector<uint8_t> data_;
};

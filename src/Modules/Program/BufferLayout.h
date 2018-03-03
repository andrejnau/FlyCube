#pragma once

class BufferLayout
{
public:
    BufferLayout(
        const char* src_data,
        size_t buffer_size,
        std::vector<size_t>&& data_size,
        std::vector<size_t>&& src_offset,
        std::vector<size_t>&& dst_offset
    )
        : src_data(src_data)
        , dst_data(buffer_size)
        , data_size(std::move(data_size))
        , src_offset(std::move(src_offset))
        , dst_offset(std::move(dst_offset))
    {
    }

    bool SyncData()
    {
        bool dirty = false;
        for (size_t i = 0; i < data_size.size(); ++i)
        {
            const char* ptr_src = src_data + src_offset[i];
            char* ptr_dst = dst_data.data() + dst_offset[i];
            if (std::memcmp(ptr_dst, ptr_src, data_size[i]) != 0)
            {
                std::memcpy(ptr_dst, ptr_src, data_size[i]);
                dirty = true;
            }
        }
        return dirty;
    }

    const std::vector<char>& GetBuffer()
    {
        return dst_data;
    }

private:
    const char* src_data;
    std::vector<char> dst_data;
    std::vector<size_t> data_size;
    std::vector<size_t> src_offset;
    std::vector<size_t> dst_offset;
};

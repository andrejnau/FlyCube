#pragma once
#include <Instance/QueryInterface.h>
#include <View/View.h>
#include <memory>
#include <string>
#include <gli/format.hpp>

class Resource : public QueryInterface
{
public:
    virtual ~Resource() = default;
    virtual ResourceType GetResourceType() const = 0;
    virtual gli::format GetFormat() const = 0;
    virtual MemoryType GetMemoryType() const = 0;
    virtual uint64_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;
    virtual uint16_t GetDepthOrArraySize() const = 0;
    virtual uint16_t GetMipLevels() const = 0;
    virtual void SetName(const std::string& name) = 0;
    virtual void UpdateUploadData(const void* data, uint64_t offset, uint64_t num_bytes) = 0;
    virtual void UpdateSubresource(uint32_t texture_subresource, uint64_t buffer_offset, uint32_t buffer_row_pitch, uint32_t buffer_depth_pitch,
                                   const void* src_data, uint32_t src_row_pitch, uint32_t src_depth_pitch, uint32_t num_rows, uint32_t num_slices) = 0;

    void SetPrivateResource(uint64_t id, const std::shared_ptr<Resource>& resource)
    {
        m_private_resources[id] = resource;
    }

    std::shared_ptr<Resource>& GetPrivateResource(uint64_t id)
    {
        return m_private_resources[id];
    }

private:
    std::map<uint64_t, std::shared_ptr<Resource>> m_private_resources;
};

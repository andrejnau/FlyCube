#pragma once

#include <vulkan/vulkan.h>
#include "Context/Resource.h"

class VKResource : public Resource
{
public:
    using Ptr = std::shared_ptr<VKResource>;

    struct Image
    {
        VkImage res = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkExtent2D size = {};
        size_t level_count = 0;
    } image;

    struct Buffer
    {
        VkBuffer res = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        uint32_t size = 0;
    } buffer;

    enum class Type
    {
        kUnknown,
        kBuffer,
        kImage
    };

    Type res_type = Type::kUnknown;

    VKResource::Ptr GetUploadResource(size_t subresource)
    {
        if (subresource >= m_upload_res.size())
            m_upload_res.resize(subresource + 1);
        return m_upload_res[subresource];
    }

    virtual void SetName(const std::string& name) override
    {
    }

private:
    std::vector<VKResource::Ptr> m_upload_res;
};

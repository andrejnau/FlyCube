#pragma once

#include <vulkan/vulkan.h>
#include "Context/Resource.h"

class VKDescriptorHeapRange;

using VKBindKey = std::tuple<size_t /*program_id*/, ShaderType /*shader_type*/, VkDescriptorType /*res_type*/, uint32_t /*slot*/>;

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
        size_t msaa_count = 1;
        size_t array_layers = 1;
    } image;

    struct Buffer
    {
        VkBuffer res = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        uint32_t size = 0;
    } buffer;

    struct Sampler
    {
        VkSampler res;
    } sampler;

    enum class Type
    {
        kUnknown,
        kBuffer,
        kImage,
        kSampler
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

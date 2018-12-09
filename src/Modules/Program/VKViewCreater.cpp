#include "VKViewCreater.h"
#include <Utilities/State.h>
#include <Texture/DXGIFormatHelper.h>
#include <memory>

VKViewCreater::VKViewCreater(VKContext& context, const IShaderBlobProvider& shader_provider)
    : m_context(context)
    , m_shader_provider(shader_provider)
{
}

VKView::Ptr VKViewCreater::CreateView()
{
    return std::make_shared<VKView>();
}

void VKViewCreater::OnLinkProgram()
{
    auto shader_types = m_shader_provider.GetShaderTypes();
    for (auto & shader_type : shader_types)
    {
        auto spirv = m_shader_provider.GetBlobByType(shader_type);
        assert(spirv.size % 4 == 0);
        std::vector<uint32_t> spirv32((uint32_t*)spirv.data, ((uint32_t*)spirv.data) + spirv.size / 4);

        ParseShader(shader_type, spirv32);
    }
}

void VKViewCreater::ParseShader(ShaderType shader_type, const std::vector<uint32_t>& spirv_binary)
{
    m_shader_ref.emplace(shader_type, spirv_binary);
    spirv_cross::CompilerHLSL& compiler = m_shader_ref.find(shader_type)->second.compiler;
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    auto generate_bindings = [&](const std::vector<spirv_cross::Resource>& resources, VkDescriptorType res_type)
    {
        for (auto& res : resources)
        {
            auto& info = m_shader_ref.find(shader_type)->second.resources[res.name];
            info.res = res;
            auto &type = compiler.get_type(res.base_type_id);

            if (type.basetype == spirv_cross::SPIRType::BaseType::Image && type.image.dim == spv::Dim::DimBuffer)
            {
                if (res_type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
                {
                    res_type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                }
                else if (res_type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                {
                    res_type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                }
            }

            info.descriptor_type = res_type;
        }
    };

    generate_bindings(resources.uniform_buffers, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    generate_bindings(resources.separate_images, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    generate_bindings(resources.separate_samplers, VK_DESCRIPTOR_TYPE_SAMPLER);
    generate_bindings(resources.storage_buffers, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    generate_bindings(resources.storage_images, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
}

VKView::Ptr VKViewCreater::GetEmptyDescriptor(ResourceType res_type)
{
    static std::map<ResourceType, VKView::Ptr> empty_handles;
    auto it = empty_handles.find(res_type);
    if (it == empty_handles.end())
        it = empty_handles.emplace(res_type, CreateView()).first;
    return std::static_pointer_cast<VKView>(it->second);
}

VKView::Ptr VKViewCreater::GetView(uint32_t m_program_id, ShaderType shader_type, ResourceType res_type, uint32_t slot, const std::string& name, const Resource::Ptr& ires)
{
    if (!ires)
        return GetEmptyDescriptor(res_type);

    VKResource& res = static_cast<VKResource&>(*ires);

    View::Ptr& view = ires->GetView({ m_shader_provider.GetProgramId(), shader_type, res_type, slot });
    if (view)
        return std::static_pointer_cast<VKView>(view);

    view = CreateView();

    VKView& handle = static_cast<VKView&>(*view);

    switch (res_type)
    {
    case ResourceType::kSrv:
        CreateSrv(shader_type, name, slot, res, handle);
        break;
    case ResourceType::kUav:
        CreateUAV(shader_type, name, slot, res, handle);
        break;
    case ResourceType::kCbv:
        CreateCBV(shader_type, slot, res, handle);
        break;
    case ResourceType::kSampler:
        CreateSampler(shader_type, slot, res, handle);
        break;
    case ResourceType::kRtv:
        CreateRTV(slot, res, handle);
        break;
    case ResourceType::kDsv:
        CreateDSV(res, handle);
        break;
    }

    return std::static_pointer_cast<VKView>(view);
}

void VKViewCreater::CreateSrv(ShaderType type, const std::string& name, uint32_t slot, const VKResource& res, VKView& handle)
{
    ShaderRef& shader_ref = m_shader_ref.find(type)->second;
    auto ref_res = shader_ref.resources[name];

    auto &res_type = shader_ref.compiler.get_type(ref_res.res.base_type_id);

    auto dim = res_type.image.dim;
    if (res_type.basetype == spirv_cross::SPIRType::BaseType::Struct)
        dim = spv::Dim::DimBuffer;

    bool is_buffer = false;
    switch (dim)
    {
    case spv::Dim::Dim2D:
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = res.image.res;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = res.image.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = res.image.level_count;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = res.image.array_layers;

        if (vkCreateImageView(m_context.m_device, &viewInfo, nullptr, &handle.srv) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }
        break;
    }
    case spv::Dim::DimCube:
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = res.image.res;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.format = res.image.format;
        viewInfo.subresourceRange.aspectMask = m_context.GetAspectFlags(res.image.format);
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = res.image.array_layers;

        if (vkCreateImageView(m_context.m_device, &viewInfo, nullptr, &handle.srv) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        break;
    }
    default:
    {
        bool todo = true;
        break;
    }
    }
}

void VKViewCreater::CreateUAV(ShaderType type, const std::string& name, uint32_t slot, const VKResource& res, VKView& handle)
{
    ShaderRef& shader_ref = m_shader_ref.find(type)->second;
    auto ref_res = shader_ref.resources[name];

    auto &res_type = shader_ref.compiler.get_type(ref_res.res.base_type_id);

    auto dim = res_type.image.dim;

    switch (dim)
    {
    case spv::Dim::Dim2D:
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = res.image.res;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = res.image.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = res.image.level_count;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_context.m_device, &viewInfo, nullptr, &handle.srv) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }
        break;
    }
    default:
    {
        bool todo = true;
        break;
    }
    }
}

void VKViewCreater::CreateCBV(ShaderType type, uint32_t slot, const VKResource& res, VKView& handle)
{
}

void VKViewCreater::CreateSampler(ShaderType type, uint32_t slot, const VKResource& res, VKView& handle)
{
}

void VKViewCreater::CreateRTV(uint32_t slot, const VKResource& res, VKView& handle)
{
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = res.image.res;
    createInfo.format = res.image.format;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(m_context.m_device, &createInfo, nullptr, &handle.rtv);
}

static bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void VKViewCreater::CreateDSV(const VKResource& res, VKView& handle)
{
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = res.image.res;
    createInfo.format = res.image.format;
    if (res.image.array_layers == 6)
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    else
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (hasStencilComponent(res.image.format))
        createInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = res.image.array_layers;
    vkCreateImageView(m_context.m_device, &createInfo, nullptr, &handle.dsv);
}

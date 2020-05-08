#include "VKViewCreater.h"
#include <Utilities/State.h>
#include <Utilities/VKUtility.h>
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

    auto generate_bindings = [&](const spirv_cross::SmallVector<spirv_cross::Resource>& resources, vk::DescriptorType res_type)
    {
        for (auto& res : resources)
        {
            auto& info = m_shader_ref.find(shader_type)->second.resources[res.name];
            info.res = res;
            auto &type = compiler.get_type(res.base_type_id);

            if (type.basetype == spirv_cross::SPIRType::BaseType::Image && type.image.dim == spv::Dim::DimBuffer)
            {
                if (res_type == vk::DescriptorType::eSampledImage)
                {
                    res_type = vk::DescriptorType::eUniformTexelBuffer;
                }
                else if (res_type == vk::DescriptorType::eStorageImage)
                {
                    res_type = vk::DescriptorType::eStorageTexelBuffer;
                }
            }

            info.descriptor_type = res_type;
        }
    };

    generate_bindings(resources.uniform_buffers, vk::DescriptorType::eUniformBuffer);
    generate_bindings(resources.separate_images, vk::DescriptorType::eSampledImage);
    generate_bindings(resources.separate_samplers, vk::DescriptorType::eSampler);
    generate_bindings(resources.storage_buffers, vk::DescriptorType::eStorageBuffer);
    generate_bindings(resources.storage_images, vk::DescriptorType::eStorageImage);
    generate_bindings(resources.acceleration_structures, vk::DescriptorType::eAccelerationStructureNV);
}

VKView::Ptr VKViewCreater::GetEmptyDescriptor(ResourceType res_type)
{
    static std::map<ResourceType, VKView::Ptr> empty_handles;
    auto it = empty_handles.find(res_type);
    if (it == empty_handles.end())
        it = empty_handles.emplace(res_type, CreateView()).first;
    return std::static_pointer_cast<VKView>(it->second);
}

VKView::Ptr VKViewCreater::GetView(uint32_t m_program_id, ShaderType shader_type, ResourceType res_type, uint32_t slot, const ViewDesc& view_desc, const std::string& name, const Resource::Ptr& ires)
{
    if (!ires)
        return GetEmptyDescriptor(res_type);

    VKResource& res = static_cast<VKResource&>(*ires);

    View::Ptr& iview = ires->GetView({m_program_id, shader_type, res_type, slot}, view_desc);
    if (iview)
        return std::static_pointer_cast<VKView>(iview);

    VKView::Ptr view = CreateView();
    iview = view;

    switch (res_type)
    {
    case ResourceType::kSrv:
    case ResourceType::kUav:
        CreateSrv(shader_type, name, slot, view_desc, res, *view);
        break;
    case ResourceType::kRtv:
    case ResourceType::kDsv:
        CreateRTV(slot, view_desc, res, *view);
        break;
    }

    return view;
}

void VKViewCreater::CreateSrv(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const VKResource& res, VKView& handle)
{
    ShaderRef& shader_ref = m_shader_ref.find(type)->second;
    auto& ref_res = shader_ref.resources[name];
    auto& res_type = shader_ref.compiler.get_type(ref_res.res.type_id);
    auto& dim = res_type.image.dim;

    if (res_type.basetype == spirv_cross::SPIRType::BaseType::Struct ||
        res_type.basetype == spirv_cross::SPIRType::BaseType::AccelerationStructureNV)
        return;

    vk::ImageViewCreateInfo view_info = {};
    view_info.image = res.image.res.get();
    view_info.format = res.image.format;
    view_info.subresourceRange.aspectMask = m_context.GetAspectFlags(view_info.format);
    view_info.subresourceRange.baseMipLevel = view_desc.level;
    if (view_desc.count == -1)
        view_info.subresourceRange.levelCount = res.image.level_count - view_info.subresourceRange.baseMipLevel;
    else
        view_info.subresourceRange.levelCount = view_desc.count;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = res.image.array_layers;

    switch (dim)
    {
    case spv::Dim::Dim1D:
    {
        if (res_type.image.arrayed)
            view_info.viewType = vk::ImageViewType::e1DArray;
        else
            view_info.viewType = vk::ImageViewType::e1D;
        break;
    }
    case spv::Dim::Dim2D:
    {
        if (res_type.image.arrayed)
            view_info.viewType = vk::ImageViewType::e2DArray;
        else
            view_info.viewType = vk::ImageViewType::e2D;
        break;
    }
    case spv::Dim::Dim3D:
    {
        view_info.viewType = vk::ImageViewType::e3D;
        break;
    }
    case spv::Dim::DimCube:
    {
        if (res_type.image.arrayed)
            view_info.viewType = vk::ImageViewType::eCubeArray;
        else
            view_info.viewType = vk::ImageViewType::eCube;
        break;
    }   
    default:
    {
        ASSERT(false);
        break;
    }
    }
    handle.srv = m_context.m_vk_device.createImageViewUnique(view_info);
}

void VKViewCreater::CreateRTV(uint32_t slot, const ViewDesc& view_desc, const VKResource& res, VKView& handle)
{
    vk::ImageViewCreateInfo view_info = {};
    view_info.image = res.image.res.get();
    view_info.format = res.image.format;
    if (res.image.array_layers > 1)
        view_info.viewType = vk::ImageViewType::e2DArray;
    else
        view_info.viewType = vk::ImageViewType::e2D;
    view_info.subresourceRange.aspectMask = m_context.GetAspectFlags(view_info.format);
    view_info.subresourceRange.baseMipLevel = view_desc.level;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = res.image.array_layers;

    handle.om = m_context.m_vk_device.createImageViewUnique(view_info);
}

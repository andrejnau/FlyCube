#pragma once

#include <Context/VKContext.h>
#include <Context/VKResource.h>
#include <Context/View.h>
#include <Context/VKView.h>
#include <Context/DescriptorPool.h>
#include "IShaderBlobProvider.h"

#include <spirv_cross.hpp>
#include <spirv_hlsl.hpp>

class VKViewCreater
{
public:
    VKViewCreater(VKContext& context, const IShaderBlobProvider& shader_provider);

    VKView::Ptr CreateView();

    void OnLinkProgram();

    void ParseShader(ShaderType shader_type, const std::vector<uint32_t>& spirv_binary);

    VKView::Ptr GetView(uint32_t program_id, ShaderType shader_type, ResourceType res_type, uint32_t slot, const ViewDesc& view_desc, const std::string& name, const Resource::Ptr& ires);

private:
    VKView::Ptr GetEmptyDescriptor(ResourceType res_type);

    void CreateSrv(ShaderType type, const std::string& name, uint32_t slot, const VKResource& res, VKView& handle);
    void CreateUAV(ShaderType type, const std::string& name, uint32_t slot, const VKResource& ires, VKView& handle);
    void CreateCBV(ShaderType type, uint32_t slot, const VKResource& res, VKView& handle);
    void CreateSampler(ShaderType type, uint32_t slot, const VKResource& res, VKView& handle);
    void CreateRTV(uint32_t slot, const VKResource& res, VKView& handle);
    void CreateDSV(const VKResource& res, VKView& handle);

    VKContext& m_context;
    const IShaderBlobProvider& m_shader_provider;

    struct ShaderRef
    {
        ShaderRef(const std::vector<uint32_t>& spirv_binary)
            : compiler(spirv_cross::CompilerHLSL(std::move(spirv_binary)))
        {
        }

        spirv_cross::CompilerHLSL compiler;

        struct ResourceRef
        {
            spirv_cross::Resource res;
            VkDescriptorType descriptor_type;
            uint32_t binding;
        };

        std::map<std::string, ResourceRef> resources;
    };


    std::map<ShaderType, ShaderRef> m_shader_ref;
};

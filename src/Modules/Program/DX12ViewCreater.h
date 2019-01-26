#pragma once

#include <Context/DX12Context.h>
#include <Resource/DX12Resource.h>
#include <View/View.h>
#include <Context/DescriptorPool.h>
#include "IShaderBlobProvider.h"

class DX12ViewCreater
{
public:
    DX12ViewCreater(DX12Context& context, const IShaderBlobProvider& shader_provider);

    DX12View::Ptr GetView(const BindKey& bind_key, const ViewDesc& view_desc, const std::string& name, const Resource::Ptr& ires);

private:
    DX12View::Ptr GetEmptyDescriptor(ResourceType res_type);

    void CreateSrv(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const DX12Resource& res, DX12View& handle);
    void CreateUAV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const DX12Resource& ires, DX12View& handle);
    void CreateCBV(ShaderType type, uint32_t slot, const DX12Resource& res, DX12View& handle);
    void CreateSampler(ShaderType type, uint32_t slot, const DX12Resource& res, DX12View& handle);
    void CreateRTV(uint32_t slot, const ViewDesc& view_desc, const DX12Resource& res, DX12View& handle);
    void CreateDSV(const ViewDesc& view_desc, const DX12Resource& res, DX12View& handle);

    decltype(&::D3DReflect) _D3DReflect = &::D3DReflect;
    DX12Context& m_context;
    const IShaderBlobProvider& m_shader_provider;
};

#pragma once

#include <Context/DX12Context.h>
#include <Context/DX12Resource.h>
#include <Context/View.h>
#include "IShaderBlobProvider.h"

class DX12ViewCreater
{
public:
    DX12ViewCreater(DX12Context& context, const IShaderBlobProvider& shader_provider);

    DescriptorHeapRange::Ptr GetView(ShaderType shader_type, const std::string& name, ResourceType res_type, uint32_t slot, const Resource::Ptr& ires);

private:
    DescriptorHeapRange::Ptr GetEmptyDescriptor(ResourceType res_type);

    void CreateSrv(ShaderType type, const std::string& name, uint32_t slot, const DX12Resource& res, DescriptorHeapRange& handle);
    void CreateUAV(ShaderType type, const std::string& name, uint32_t slot, const DX12Resource& ires, DescriptorHeapRange& handle);
    void CreateCBV(ShaderType type, uint32_t slot, const DX12Resource& res, DescriptorHeapRange& handle);
    void CreateSampler(ShaderType type, uint32_t slot, const DX12Resource& res, DescriptorHeapRange& handle);
    void CreateRTV(uint32_t slot, const DX12Resource& res, DescriptorHeapRange& handle);
    void CreateDSV(const DX12Resource& res, DescriptorHeapRange& handle);

    decltype(&::D3DReflect) _D3DReflect = &::D3DReflect;
    DX12Context& m_context;
    const IShaderBlobProvider& m_shader_provider;
};

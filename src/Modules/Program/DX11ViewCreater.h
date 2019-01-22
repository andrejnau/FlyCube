#pragma once

#include <Context/DX11Context.h>
#include <Context/DX11Resource.h>
#include <Context/View.h>
#include <Context/DX11View.h>
#include <Context/DescriptorPool.h>
#include "IShaderBlobProvider.h"

class DX11ViewCreater
{
public:
    DX11ViewCreater(DX11Context& context, const IShaderBlobProvider& shader_provider);

    DX11View::Ptr GetView(const BindKey& bind_key, const ViewDesc& view_desc, const std::string& name, const Resource::Ptr& ires);

private:
    ComPtr<ID3D11ShaderResourceView> CreateSrv(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr& ires);
    ComPtr<ID3D11UnorderedAccessView> CreateUAV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr& ires);
    ComPtr<ID3D11DepthStencilView> CreateDsv(const ViewDesc& view_desc, const Resource::Ptr& ires);
    ComPtr<ID3D11RenderTargetView> CreateRtv(uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr& ires);

    decltype(&::D3DReflect) _D3DReflect = &::D3DReflect;
    DX11Context& m_context;
    const IShaderBlobProvider& m_shader_provider;
    size_t m_program_id = 0;
    std::map<ShaderType, ShaderBlob> m_blob_map;
};

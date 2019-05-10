#pragma once

#include <Context/DX11Context.h>
#include <Resource/DX11Resource.h>
#include <View/View.h>
#include <View/DX11View.h>
#include <Context/DescriptorPool.h>
#include "IShaderBlobProvider.h"

class DX11ViewCreater
{
public:
    DX11ViewCreater(DX11Context& context, const IShaderBlobProvider& shader_provider);

    DX11View::Ptr GetView(const BindKey& bind_key, const ViewDesc& view_desc, const std::string& name, const Resource::Ptr& ires);

private:
    void CreateSrv(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const DX11Resource::Ptr& res, ComPtr<ID3D11ShaderResourceView>& srv);
    void CreateUAV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const DX11Resource::Ptr& res, ComPtr<ID3D11UnorderedAccessView>& uav);
    void CreateDsv(const ViewDesc& view_desc, const DX11Resource::Ptr& res, ComPtr<ID3D11DepthStencilView>& rtv);
    void CreateRtv(uint32_t slot, const ViewDesc& view_desc, const DX11Resource::Ptr& res, ComPtr<ID3D11RenderTargetView>& dsv);

    DX11Context& m_context;
    const IShaderBlobProvider& m_shader_provider;
    size_t m_program_id = 0;
    std::map<ShaderType, ShaderBlob> m_blob_map;
};

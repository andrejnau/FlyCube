#include "GenerateMipmapPass.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <algorithm>

GenerateMipmapPass::GenerateMipmapPass(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program(context.device)
{
    D3D11_SAMPLER_DESC samp_desc = {};
    samp_desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    samp_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samp_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samp_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samp_desc.MinLOD = 0;
    samp_desc.MaxLOD = D3D11_FLOAT32_MAX;

    ASSERT_SUCCEEDED(m_context.device->CreateSamplerState(&samp_desc, &m_shadow_sampler));
    m_context.device_context->CSSetSamplers(0, 1, m_shadow_sampler.GetAddressOf());
}

void GenerateMipmapPass::OnUpdate()
{
}

void GenerateMipmapPass::OnRender()
{
    D3D11_SHADER_RESOURCE_VIEW_DESC orig_desc = {};
    m_input.shadow_pass.srv->GetDesc(&orig_desc);

    ComPtr<ID3D11Resource> texture;
    m_input.shadow_pass.srv->GetResource(&texture);

    uint32_t mipmaps = orig_desc.TextureCube.MipLevels;
    uint32_t width = 2048;
    uint32_t height = 2048;

    m_program.UseProgram(m_context.device_context);

    D3D11_TEXTURE2D_DESC texture_desc = {};
    texture_desc.Width = width;
    texture_desc.Height = height;
    texture_desc.MipLevels = orig_desc.TextureCube.MipLevels;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;

    ComPtr<ID3D11Texture2D> uav_texture;
    ASSERT_SUCCEEDED(m_context.device->CreateTexture2D(&texture_desc, nullptr, &uav_texture));   

    for (uint32_t array_slice = 0; array_slice < 6; ++array_slice)
    {
        m_context.device_context->CopySubresourceRegion(uav_texture.Get(), 0, 0, 0, 0, texture.Get(), D3D11CalcSubresource(0, array_slice, orig_desc.TextureCube.MipLevels), nullptr);

        for (uint32_t top_mip = 0; top_mip < mipmaps - 1; ++top_mip)
        {
            uint32_t dst_width = std::max(width >> (top_mip + 1), 1u);
            uint32_t dst_height = std::max(height >> (top_mip + 1), 1u);

            D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
            srv_desc.Format = orig_desc.Format;
            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srv_desc.Texture2D.MipLevels = 1;
            srv_desc.Texture2D.MostDetailedMip = top_mip;

            ComPtr<ID3D11ShaderResourceView> srv;
            m_context.device->CreateShaderResourceView(uav_texture.Get(), &srv_desc, &srv);

            D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
            uav_desc.Format = orig_desc.Format;
            uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            uav_desc.Texture2D.MipSlice = top_mip + 1;

            ComPtr<ID3D11UnorderedAccessView> uav;
            m_context.device->CreateUnorderedAccessView(uav_texture.Get(), &uav_desc, &uav);

            m_context.device_context->CSSetUnorderedAccessViews(0, 1, uav.GetAddressOf(), nullptr);
            m_context.device_context->CSSetShaderResources(m_program.cs.texture.SrcTexture, 1, srv.GetAddressOf());

            m_program.cs.cbuffer.CB.TexelSize = glm::vec2(1.0f / dst_width, 1.0f / dst_height);
            m_program.cs.UpdateCBuffers(m_context.device_context);

            m_context.device_context->Dispatch(std::max(dst_width / 8, 1u), std::max(dst_height / 8, 1u), 1);
            m_context.device_context->CopySubresourceRegion(texture.Get(), 
                D3D11CalcSubresource(top_mip + 1, array_slice, orig_desc.TextureCube.MipLevels), 0, 0, 0, 
                uav_texture.Get(), D3D11CalcSubresource(top_mip + 1, 0, orig_desc.TextureCube.MipLevels), nullptr);
        }
    }
}

void GenerateMipmapPass::OnResize(int width, int height)
{
}

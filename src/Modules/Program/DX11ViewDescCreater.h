#pragma once

#include <Context/DX11Context.h>
#include <Resource/DX11Resource.h>
#include <Utilities/State.h>
#include <Texture/DXGIFormatHelper.h>
#include <d3d11shader.h>

D3D11_SHADER_RESOURCE_VIEW_DESC DX11GeSRVDesc(const D3D11_SHADER_INPUT_BIND_DESC& binding_desc, const ViewDesc& view_desc, const ComPtr<ID3D11Resource>& resource);
D3D11_UNORDERED_ACCESS_VIEW_DESC DX11GetUAVDesc(const D3D11_SHADER_INPUT_BIND_DESC& binding_desc, const ViewDesc& view_desc, const ComPtr<ID3D11Resource>& resource);
D3D11_RENDER_TARGET_VIEW_DESC DX11GetRTVDesc(const D3D11_SIGNATURE_PARAMETER_DESC& binding_desc, const ViewDesc& view_desc, const ComPtr<ID3D11Resource>& resource);
D3D11_DEPTH_STENCIL_VIEW_DESC DX11GetDSVDesc(const ViewDesc& view_desc, const ComPtr<ID3D11Resource>& resource);


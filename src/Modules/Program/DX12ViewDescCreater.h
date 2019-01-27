#pragma once

#include <Context/DX12Context.h>
#include <Resource/DX12Resource.h>
#include <Utilities/State.h>
#include <Texture/DXGIFormatHelper.h>

D3D12_SHADER_RESOURCE_VIEW_DESC DX12GeSRVDesc(const D3D12_SHADER_INPUT_BIND_DESC& binding_desc, const ViewDesc& view_desc, const DX12Resource& res);
D3D12_UNORDERED_ACCESS_VIEW_DESC DX12GetUAVDesc(const D3D12_SHADER_INPUT_BIND_DESC& binding_desc, const ViewDesc& view_desc, const DX12Resource& res);
D3D12_RENDER_TARGET_VIEW_DESC DX12GetRTVDesc(const D3D12_SIGNATURE_PARAMETER_DESC& binding_desc, const ViewDesc& view_desc, const DX12Resource& res);
D3D12_DEPTH_STENCIL_VIEW_DESC DX12GetDSVDesc(const ViewDesc& view_desc, const DX12Resource& res);

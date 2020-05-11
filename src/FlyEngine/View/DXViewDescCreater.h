#pragma once
#include <Utilities/State.h>
#include <d3d12shader.h>
#include <Shader/DXReflector.h>
#include <d3d12.h>
#include "View/ViewDesc.h"

D3D12_SHADER_RESOURCE_VIEW_DESC DX12GeSRVDesc(const ViewDesc& view_desc, const D3D12_RESOURCE_DESC& res_desc);
D3D12_UNORDERED_ACCESS_VIEW_DESC DX12GetUAVDesc(const ViewDesc& view_desc, const D3D12_RESOURCE_DESC& res_desc);
D3D12_RENDER_TARGET_VIEW_DESC DX12GetRTVDesc(const ViewDesc& view_desc, const D3D12_RESOURCE_DESC& res_desc);
D3D12_DEPTH_STENCIL_VIEW_DESC DX12GetDSVDesc(const ViewDesc& view_desc, const D3D12_RESOURCE_DESC& res_desc);

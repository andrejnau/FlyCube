#pragma once

#include "View.h"

class DX11View : public View
{
public:
    using Ptr = std::shared_ptr<DX11View>;

    ComPtr<ID3D11ShaderResourceView> srv;
    ComPtr<ID3D11UnorderedAccessView> uav;
    ComPtr<ID3D11RenderTargetView> rtv;
    ComPtr<ID3D11DepthStencilView> dsv;
};

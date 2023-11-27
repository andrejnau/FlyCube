#pragma once
#include "Instance.h"

#include <dxgi1_4.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXInstance : public Instance {
public:
    DXInstance();
    std::vector<std::shared_ptr<Adapter>> EnumerateAdapters() override;
    ComPtr<IDXGIFactory4> GetFactory();

private:
    ComPtr<IDXGIFactory4> m_dxgi_factory;
};

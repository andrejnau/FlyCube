#pragma once
#include "Adapter/Adapter.h"

#include <dxgi.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXInstance;

class DXAdapter : public Adapter {
public:
    DXAdapter(DXInstance& instance, const ComPtr<IDXGIAdapter1>& adapter);
    const std::string& GetName() const override;
    std::shared_ptr<Device> CreateDevice() override;
    DXInstance& GetInstance();
    ComPtr<IDXGIAdapter1> GetAdapter();

private:
    DXInstance& m_instance;
    ComPtr<IDXGIAdapter1> m_adapter;
    std::string m_name;
};

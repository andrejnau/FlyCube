#pragma once
#include "Adapter/Adapter.h"

#include <dxgi1_6.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

class DXInstance;

class DXAdapter : public Adapter {
public:
    DXAdapter(DXInstance& instance, const ComPtr<IDXGIAdapter1>& adapter);
    const std::string& GetName() const override;
    std::shared_ptr<Device> CreateDevice() override;
    DXInstance& GetInstance();
    ComPtr<IDXGIAdapter1> GetAdapter();

private:
    DXInstance& instance_;
    ComPtr<IDXGIAdapter1> adapter_;
    std::string name_;
};

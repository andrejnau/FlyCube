#include "Adapter/Adapter.h"
#include <dxgi.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXAdapter : public Adapter
{
public:
    DXAdapter(const ComPtr<IDXGIAdapter1>& adapter);
    const std::string& GetName() const override;
    std::unique_ptr<Device> CreateDevice() override;

private:
    ComPtr<IDXGIAdapter1> m_adapter;
    std::string m_name;
};

#include "CommandList/CommandList.h"
#include <dxgi.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;

class DXCommandList : public CommandList
{
public:
    DXCommandList(DXDevice& device);
    void Clear(std::shared_ptr<Resource> iresource, const std::array<float, 4>& color) override;
};

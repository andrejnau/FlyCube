#include "CommandList/DXCommandList.h"
#include <Device/DXDevice.h>
#include <Utilities/DXUtility.h>
#include <Utilities/FileUtility.h>
#include <dxgi1_6.h>
#include <d3d12.h>

DXCommandList::DXCommandList(DXDevice& device)
{
}

void DXCommandList::Clear(std::shared_ptr<Resource> iresource, const std::array<float, 4>& color)
{
}

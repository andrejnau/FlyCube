#include "Program/DXProgram.h"

#include "BindingSet/DXBindingSet.h"
#include "Device/DXDevice.h"
#include "View/DXView.h"

#include <directx/d3dx12.h>

#include <deque>
#include <set>
#include <stdexcept>

DXProgram::DXProgram(DXDevice& device, const std::vector<std::shared_ptr<Shader>>& shaders)
    : ProgramBase(shaders)
{
}

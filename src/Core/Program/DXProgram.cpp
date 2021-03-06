#include "Program/DXProgram.h"
#include <Device/DXDevice.h>
#include <HLSLCompiler/DXReflector.h>
#include <View/DXView.h>
#include <BindingSet/DXBindingSet.h>
#include <deque>
#include <set>
#include <stdexcept>
#include <d3dx12.h>

DXProgram::DXProgram(DXDevice& device, const std::vector<std::shared_ptr<Shader>>& shaders)
    : ProgramBase(shaders)
{
}

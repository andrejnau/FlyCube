#include "Program/VKProgram.h"

#include "BindingSet/VKBindingSet.h"
#include "Device/VKDevice.h"
#include "Shader/ShaderBase.h"
#include "View/VKView.h"

VKProgram::VKProgram(VKDevice& device, const std::vector<std::shared_ptr<Shader>>& shaders)
    : ProgramBase(shaders)
{
}

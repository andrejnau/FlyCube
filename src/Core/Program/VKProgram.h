#pragma once
#include "ProgramBase.h"
#include <Shader/Shader.h>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <map>
#include <set>
#include <spirv_cross.hpp>
#include <spirv_hlsl.hpp>

class VKDevice;

class VKProgram : public ProgramBase
{
public:
    VKProgram(VKDevice& device, const std::vector<std::shared_ptr<Shader>>& shaders);
};

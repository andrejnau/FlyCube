#pragma once
#include "ProgramBase.h"
#include "Shader/Shader.h"

#include <vulkan/vulkan.hpp>

#include <map>
#include <set>
#include <vector>

class VKDevice;

class VKProgram : public ProgramBase {
public:
    VKProgram(VKDevice& device, const std::vector<std::shared_ptr<Shader>>& shaders);
};

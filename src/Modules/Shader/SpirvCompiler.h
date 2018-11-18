#pragma once

#include <stdint.h>
#include <vector>
#include "Shader/ShaderBase.h"

struct SpirvOption
{
    bool invert_y = true;
    bool auto_map_bindings = false;
    bool hlsl_iomap = false;
    uint32_t resource_set_binding = -1;
};

std::vector<uint32_t> SpirvCompile(const ShaderBase& shader, const SpirvOption& option = SpirvOption());

#pragma once

#include <stdint.h>
#include <vector>
#include <Instance/BaseTypes.h>

std::vector<uint32_t> SpirvCompile(const ShaderDesc& shader);

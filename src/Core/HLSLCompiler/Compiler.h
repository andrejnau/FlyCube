#pragma once
#include "Instance/BaseTypes.h"

std::vector<uint8_t> Compile(const ShaderDesc& shader, ShaderBlobType blob_type);

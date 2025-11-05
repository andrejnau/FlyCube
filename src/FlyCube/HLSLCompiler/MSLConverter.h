#pragma once
#include "Instance/BaseTypes.h"

std::string GetMSLShader(ShaderType shader_type,
                         const std::vector<uint8_t>& blob,
                         std::map<BindKey, uint32_t>& mapping);
std::string GetMSLShader(const ShaderDesc& shader, std::map<BindKey, uint32_t>& mapping);

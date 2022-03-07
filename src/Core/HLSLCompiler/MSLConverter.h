#pragma once
#include <Instance/BaseTypes.h>

std::string GetMSLShader(const std::vector<uint8_t>& blob);
std::string GetMSLShader(const ShaderDesc& shader);

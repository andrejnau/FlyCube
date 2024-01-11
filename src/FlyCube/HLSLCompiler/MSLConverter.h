#pragma once
#include "Instance/BaseTypes.h"

bool UseArgumentBuffers();
std::string GetMSLShader(const std::vector<uint8_t>& blob, std::map<std::string, uint32_t>& mapping);
std::string GetMSLShader(const ShaderDesc& shader, std::map<std::string, uint32_t>& mapping);

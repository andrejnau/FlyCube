#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include "Shader/ShaderBase.h"
#include "Shader/SpirvCompiler.h"

std::string GetGLSLShader(const ShaderBase& shader, const SpirvOption& option);

#pragma once

#include <string>

#include "Program/BufferProxy.h"

class IShaderBuffer
{
public:
    virtual ~IShaderBuffer(){}
    virtual BufferProxy Uniform(const std::string& name) = 0;
};

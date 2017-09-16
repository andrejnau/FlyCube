#pragma once

#include "Program/BufferProxy.h"
#include <string>

class IShaderBuffer
{
public:
    virtual ~IShaderBuffer(){}
    virtual BufferProxy Uniform(const std::string& name) = 0;
};

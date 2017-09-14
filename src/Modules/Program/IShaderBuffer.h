#pragma once

#include <string>

#include "BufferProxy.h"

class IShaderBuffer
{
public:
    virtual ~IShaderBuffer(){}
    virtual BufferProxy Uniform(const std::string& name) = 0;
};

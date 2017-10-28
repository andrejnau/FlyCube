#pragma once

#include <memory>
#include "Program/IShaderBuffer.h"

class Program
{
public:
    using Ptr = std::shared_ptr<Program>;
    virtual IShaderBuffer& GetVSCBuffer(const std::string& name) = 0;
    virtual IShaderBuffer& GetPSCBuffer(const std::string& name) = 0;
};
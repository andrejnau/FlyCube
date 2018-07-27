#pragma once

#include <Context/BaseTypes.h>

struct ShaderBlob
{
    const uint8_t* data = nullptr;
    const size_t size = 0;

    operator bool() const
    {
        return data && size;
    }
};

class IShaderBlobProvider
{
public:
    virtual ShaderBlob GetBlobByType(ShaderType type) const = 0;
};

#pragma once

#include <Context/BaseTypes.h>
#include <set>

struct ShaderBlob
{
    const uint8_t* data = nullptr;
    const size_t size = 0;

    ShaderBlob() = default;
    ShaderBlob(const ShaderBlob&) = default;
    ShaderBlob& operator= (const ShaderBlob&) = default;

    operator bool() const
    {
        return data && size;
    }
};

class IShaderBlobProvider
{
public:
    virtual size_t GetProgramId() const = 0;
    virtual ShaderBlob GetBlobByType(ShaderType type) const = 0;
    virtual std::set<ShaderType> GetShaderTypes() const
    {
        return {};
    }
};

#pragma once
#include "Shader/ShaderBase.h"
#include <Instance/BaseTypes.h>
#include <map>
#include <string>
#import <Metal/Metal.h>

class MTShader : public ShaderBase
{
public:
    MTShader(const std::vector<uint8_t>& blob, ShaderBlobType blob_type, ShaderType shader_type);
    MTShader(const ShaderDesc& desc, ShaderBlobType blob_type);

    const std::string& GetSource() const;
    uint32_t GetIndex(BindKey bind_key) const;
    id<MTLLibrary> CreateLibrary(id<MTLDevice> device);
    id<MTLFunction> CreateFunction(id<MTLLibrary> library, const std::string& entry_point);

protected:
    std::string m_source;
    std::map<std::string, uint32_t> m_index_mapping;
};

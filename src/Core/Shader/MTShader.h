#pragma once
#include "Shader/ShaderBase.h"
#include <Instance/BaseTypes.h>
#include <map>
#include <string>

class MTShader : public ShaderBase
{
public:
    MTShader(const ShaderDesc& desc, ShaderBlobType blob_type);

    const std::string& GetSource() const;
    uint32_t GetIndex(BindKey bind_key) const;

protected:
    std::string m_source;
    std::map<std::string, uint32_t> m_index_mapping;
};

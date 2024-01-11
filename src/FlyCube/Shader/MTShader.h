#pragma once
#include "Instance/BaseTypes.h"
#include "Shader/ShaderBase.h"

#import <Metal/Metal.h>

#include <map>
#include <string>

class MTDevice;

class MTShader : public ShaderBase {
public:
    MTShader(MTDevice& device, const std::vector<uint8_t>& blob, ShaderBlobType blob_type, ShaderType shader_type);
    MTShader(MTDevice& device, const ShaderDesc& desc, ShaderBlobType blob_type);

    const std::string& GetSource() const;
    uint32_t GetIndex(BindKey bind_key) const;
    id<MTLLibrary> GetLibrary() const;
    id<MTLFunction> CreateFunction(id<MTLLibrary> library, const std::string& entry_point);

private:
    void CreateLibrary();

    MTDevice& m_device;
    id<MTLLibrary> m_library;
};

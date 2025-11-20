#pragma once
#include "Instance/BaseTypes.h"
#include "Shader/ShaderBase.h"

#import <Metal/Metal.h>

#include <map>
#include <vector>

class MTDevice;

class MTShader : public ShaderBase {
public:
    MTShader(MTDevice& device, const std::vector<uint8_t>& blob, ShaderBlobType blob_type, ShaderType shader_type);

    uint32_t GetIndex(BindKey bind_key) const;
    MTL4LibraryFunctionDescriptor* GetFunctionDescriptor();

private:
    MTL4LibraryFunctionDescriptor* function_descriptor_ = nullptr;
    std::map<BindKey, uint32_t> slot_remapping_;
};

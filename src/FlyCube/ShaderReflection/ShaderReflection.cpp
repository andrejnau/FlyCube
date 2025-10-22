#include "ShaderReflection/ShaderReflection.h"

#include "ShaderReflection/DXILReflection.h"
#include "ShaderReflection/SPIRVReflection.h"
#include "Utilities/NotReached.h"

std::shared_ptr<ShaderReflection> CreateShaderReflection(ShaderBlobType type, const void* data, size_t size)
{
    switch (type) {
    case ShaderBlobType::kDXIL:
        return std::make_shared<DXILReflection>(data, size);
    case ShaderBlobType::kSPIRV:
        return std::make_shared<SPIRVReflection>(data, size);
    default:
        NOTREACHED();
    }
}

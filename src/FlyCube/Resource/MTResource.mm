#include "Resource/MTResource.h"

id<MTLTexture> MTResource::GetTexture() const
{
    return nullptr;
}

id<MTLBuffer> MTResource::GetBuffer() const
{
    return nullptr;
}

id<MTLSamplerState> MTResource::GetSampler() const
{
    return nullptr;
}

id<MTLAccelerationStructure> MTResource::GetAccelerationStructure() const
{
    return nullptr;
}

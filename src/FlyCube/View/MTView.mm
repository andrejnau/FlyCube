#include "View/MTView.h"

#include "Device/MTDevice.h"
#include "Resource/MTResource.h"

static MTLTextureType ConvertTextureType(ViewDimension dimension)
{
    switch (dimension) {
    case ViewDimension::kTexture1D:
        return MTLTextureType1D;
    case ViewDimension::kTexture1DArray:
        return MTLTextureType1DArray;
    case ViewDimension::kTexture2D:
        return MTLTextureType2D;
    case ViewDimension::kTexture2DMS:
        return MTLTextureType2DMultisample;
    case ViewDimension::kTexture2DArray:
        return MTLTextureType2DArray;
    case ViewDimension::kTexture2DMSArray:
        return MTLTextureType2DMultisampleArray;
    case ViewDimension::kTexture3D:
        return MTLTextureType3D;
    case ViewDimension::kTextureCube:
        return MTLTextureTypeCube;
    case ViewDimension::kTextureCubeArray:
        return MTLTextureTypeCubeArray;
    default:
        assert(false);
        return {};
    }
}

MTView::MTView(MTDevice& device, const std::shared_ptr<MTResource>& resource, const ViewDesc& view_desc)
    : m_device(device)
    , m_resource(resource)
    , m_view_desc(view_desc)
{
    if (!m_resource) {
        return;
    }
    switch (m_view_desc.view_type) {
    case ViewType::kTexture:
    case ViewType::kRWTexture:
        CreateTextureView();
        break;
    default:
        break;
    }

    if (m_view_desc.bindless) {
        decltype(auto) argument_buffer = m_device.GetBindlessArgumentBuffer();
        m_range = std::make_shared<MTGPUArgumentBufferRange>(argument_buffer.Allocate(1));
        uint64_t* arguments = static_cast<uint64_t*>(m_range->GetArgumentBuffer().contents);
        arguments[m_range->GetOffset()] = GetGpuAddress();
        id<MTLResource> resource = GetNativeResource();
        if (resource) {
            m_range->SetResourceUsage(m_range->GetOffset(), resource, GetUsage());
        }
    }
}

void MTView::CreateTextureView()
{
    decltype(auto) texture = m_resource->texture.res;
    MTLPixelFormat format = m_resource->texture.format;
    MTLTextureType texture_type = ConvertTextureType(m_view_desc.dimension);
    NSRange levels = { GetBaseMipLevel(), GetLevelCount() };
    NSRange slices = { GetBaseArrayLayer(), GetLayerCount() };

    if (m_view_desc.plane_slice == 1) {
        if (format == MTLPixelFormatDepth32Float_Stencil8) {
            format = MTLPixelFormatX32_Stencil8;
        }

        MTLTextureSwizzleChannels swizzle = MTLTextureSwizzleChannelsDefault;
        swizzle.green = MTLTextureSwizzleRed;

        m_texture_view = [texture newTextureViewWithPixelFormat:format
                                                    textureType:texture_type
                                                         levels:levels
                                                         slices:slices
                                                        swizzle:swizzle];
    } else {
        m_texture_view = [texture newTextureViewWithPixelFormat:format
                                                    textureType:texture_type
                                                         levels:levels
                                                         slices:slices];
    }

    if (m_texture_view == nullptr) {
        NSLog(@"Error: failed to create texture view");
    }
}

std::shared_ptr<Resource> MTView::GetResource()
{
    return m_resource;
}

uint32_t MTView::GetDescriptorId() const
{
    if (m_range) {
        return m_range->GetOffset();
    }
    assert(false);
    return -1;
}

uint32_t MTView::GetBaseMipLevel() const
{
    return m_view_desc.base_mip_level;
}

uint32_t MTView::GetLevelCount() const
{
    return std::min<uint32_t>(m_view_desc.level_count, m_resource->GetLevelCount() - m_view_desc.base_mip_level);
}

uint32_t MTView::GetBaseArrayLayer() const
{
    return m_view_desc.base_array_layer;
}

uint32_t MTView::GetLayerCount() const
{
    return std::min<uint32_t>(m_view_desc.layer_count, m_resource->GetLayerCount() - m_view_desc.base_array_layer);
}

std::shared_ptr<MTResource> MTView::GetMTResource() const
{
    return m_resource;
}

id<MTLTexture> MTView::GetTextureView() const
{
    return m_texture_view;
}

const ViewDesc& MTView::GetViewDesc() const
{
    return m_view_desc;
}

id<MTLResource> MTView::GetNativeResource() const
{
    if (!m_resource) {
        return {};
    }

    switch (m_view_desc.view_type) {
    case ViewType::kConstantBuffer:
    case ViewType::kBuffer:
    case ViewType::kRWBuffer:
    case ViewType::kStructuredBuffer:
    case ViewType::kRWStructuredBuffer:
        return m_resource->buffer.res;
    case ViewType::kSampler:
        return {};
    case ViewType::kTexture:
    case ViewType::kRWTexture: {
        id<MTLTexture> texture = GetTextureView();
        if (!texture) {
            texture = m_resource->texture.res;
        }
        return texture;
    }
    case ViewType::kAccelerationStructure: {
        return m_resource->acceleration_structure;
    }
    default:
        assert(false);
        return {};
    }
}

uint64_t MTView::GetGpuAddress() const
{
    if (!m_resource) {
        return 0;
    }

    switch (m_view_desc.view_type) {
    case ViewType::kConstantBuffer:
    case ViewType::kBuffer:
    case ViewType::kRWBuffer:
    case ViewType::kStructuredBuffer:
    case ViewType::kRWStructuredBuffer: {
        id<MTLBuffer> buffer = buffer = m_resource->buffer.res;
        return [buffer gpuAddress] + GetViewDesc().offset;
    }
    case ViewType::kSampler: {
        id<MTLSamplerState> sampler = sampler = m_resource->sampler.res;
        return [sampler gpuResourceID]._impl;
    }
    case ViewType::kTexture:
    case ViewType::kRWTexture: {
        id<MTLTexture> texture = GetTextureView();
        if (!texture) {
            texture = m_resource->texture.res;
        }
        return [texture gpuResourceID]._impl;
    }
    case ViewType::kAccelerationStructure: {
        id<MTLAccelerationStructure> acceleration_structure = m_resource->acceleration_structure;
        return [acceleration_structure gpuResourceID]._impl;
    }
    default:
        assert(false);
        return 0;
    }
}

MTLResourceUsage MTView::GetUsage() const
{
    switch (m_view_desc.view_type) {
    case ViewType::kAccelerationStructure:
    case ViewType::kBuffer:
    case ViewType::kConstantBuffer:
    case ViewType::kSampler:
    case ViewType::kStructuredBuffer:
    case ViewType::kTexture:
        return MTLResourceUsageRead;
    case ViewType::kRWBuffer:
    case ViewType::kRWStructuredBuffer:
    case ViewType::kRWTexture:
        return MTLResourceUsageWrite;
    default:
        assert(false);
        return MTLResourceUsageRead | MTLResourceUsageWrite;
    }
}

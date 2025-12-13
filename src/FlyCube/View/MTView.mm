#include "View/MTView.h"

#include "BindlessTypedViewPool/MTBindlessTypedViewPool.h"
#include "Device/MTDevice.h"
#include "Memory/MTMemory.h"
#include "Resource/MTResource.h"
#include "Utilities/Common.h"
#include "Utilities/Logging.h"
#include "Utilities/NotReached.h"

namespace {

MTLTextureType ConvertTextureType(ViewDimension dimension)
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
        NOTREACHED();
    }
}

} // namespace

MTView::MTView(MTDevice& device, const std::shared_ptr<MTResource>& resource, const ViewDesc& view_desc)
    : device_(device)
    , resource_(resource)
    , view_desc_(view_desc)
{
    if (!resource_) {
        return;
    }

    switch (view_desc_.view_type) {
    case ViewType::kTexture:
    case ViewType::kRWTexture:
        CreateTextureView();
        break;
    default:
        break;
    }

    if (view_desc_.view_type == ViewType::kBuffer || view_desc_.view_type == ViewType::kRWBuffer) {
        id<MTLBuffer> buffer = resource_->GetBuffer();
        MTLPixelFormat format = device_.GetMTLPixelFormat(view_desc_.buffer_format);
        uint32_t bits_per_pixel = gli::detail::bits_per_pixel(view_desc_.buffer_format) / 8;
        uint64_t size = std::min(resource_->GetWidth() - view_desc_.offset, view_desc_.buffer_size);
        uint64_t width = size / bits_per_pixel;
        MTLResourceOptions options = ConvertStorageMode(resource_->GetMemoryType()) << MTLResourceStorageModeShift;
        MTLTextureDescriptor* texture_descriptor =
            [MTLTextureDescriptor textureBufferDescriptorWithPixelFormat:format
                                                                   width:width
                                                         resourceOptions:options
                                                                   usage:MTLTextureUsageShaderRead];
        uint32_t alignment = [device_.GetDevice() minimumTextureBufferAlignmentForPixelFormat:format];
        texture_view_ = [buffer newTextureWithDescriptor:texture_descriptor
                                                  offset:view_desc_.offset
                                             bytesPerRow:Align(bits_per_pixel * width, alignment)];
    }

    if (view_desc_.bindless) {
        bindless_view_pool_ = std::make_shared<MTBindlessTypedViewPool>(device_, view_desc_.view_type, 1);
        bindless_view_pool_->WriteViewImpl(0, *this);
    }
}

void MTView::CreateTextureView()
{
    decltype(auto) texture = resource_->GetTexture();
    MTLPixelFormat format = device_.GetMTLPixelFormat(resource_->GetFormat());
    MTLTextureType texture_type = ConvertTextureType(view_desc_.dimension);
    NSRange levels = { GetBaseMipLevel(), GetLevelCount() };
    NSRange slices = { GetBaseArrayLayer(), GetLayerCount() };

    if (view_desc_.plane_slice == 1) {
        if (format == MTLPixelFormatDepth32Float_Stencil8) {
            format = MTLPixelFormatX32_Stencil8;
        }

        MTLTextureSwizzleChannels swizzle = MTLTextureSwizzleChannelsDefault;
        swizzle.green = MTLTextureSwizzleRed;

        texture_view_ = [texture newTextureViewWithPixelFormat:format
                                                   textureType:texture_type
                                                        levels:levels
                                                        slices:slices
                                                       swizzle:swizzle];
    } else {
        texture_view_ = [texture newTextureViewWithPixelFormat:format
                                                   textureType:texture_type
                                                        levels:levels
                                                        slices:slices];
    }

    if (!texture_view_) {
        Logging::Println("Failed to create MTLTexture using newTextureViewWithPixelFormat");
    }
}

std::shared_ptr<Resource> MTView::GetResource()
{
    return resource_;
}

uint32_t MTView::GetDescriptorId() const
{
    if (bindless_view_pool_) {
        return bindless_view_pool_->GetBaseDescriptorId();
    }
    NOTREACHED();
}

uint32_t MTView::GetBaseMipLevel() const
{
    return view_desc_.base_mip_level;
}

uint32_t MTView::GetLevelCount() const
{
    return std::min<uint32_t>(view_desc_.level_count, resource_->GetLevelCount() - view_desc_.base_mip_level);
}

uint32_t MTView::GetBaseArrayLayer() const
{
    return view_desc_.base_array_layer;
}

uint32_t MTView::GetLayerCount() const
{
    return std::min<uint32_t>(view_desc_.layer_count, resource_->GetLayerCount() - view_desc_.base_array_layer);
}

const ViewDesc& MTView::GetViewDesc() const
{
    return view_desc_;
}

id<MTLResource> MTView::GetNativeResource() const
{
    if (!resource_) {
        return {};
    }

    switch (view_desc_.view_type) {
    case ViewType::kConstantBuffer:
    case ViewType::kBuffer:
    case ViewType::kRWBuffer:
    case ViewType::kStructuredBuffer:
    case ViewType::kRWStructuredBuffer:
    case ViewType::kByteAddressBuffer:
    case ViewType::kRWByteAddressBuffer:
        return GetBuffer();
    case ViewType::kSampler:
        return {};
    case ViewType::kTexture:
    case ViewType::kRWTexture: {
        return GetTexture();
    }
    case ViewType::kAccelerationStructure: {
        return GetAccelerationStructure();
    }
    default:
        NOTREACHED();
    }
}

uint64_t MTView::GetGpuAddress() const
{
    if (!resource_) {
        return 0;
    }

    switch (view_desc_.view_type) {
    case ViewType::kConstantBuffer:
    case ViewType::kBuffer:
    case ViewType::kRWBuffer:
    case ViewType::kStructuredBuffer:
    case ViewType::kRWStructuredBuffer:
    case ViewType::kByteAddressBuffer:
    case ViewType::kRWByteAddressBuffer: {
        return [GetBuffer() gpuAddress] + GetViewDesc().offset;
    }
    case ViewType::kSampler: {
        return [GetSampler() gpuResourceID]._impl;
    }
    case ViewType::kTexture:
    case ViewType::kRWTexture: {
        return [GetTexture() gpuResourceID]._impl;
    }
    case ViewType::kAccelerationStructure: {
        return [GetAccelerationStructure() gpuResourceID]._impl;
    }
    default:
        NOTREACHED();
    }
}

id<MTLBuffer> MTView::GetBuffer() const
{
    if (resource_) {
        return resource_->GetBuffer();
    }
    return {};
}

id<MTLSamplerState> MTView::GetSampler() const
{
    if (resource_) {
        return resource_->GetSampler();
    }
    return {};
}

id<MTLTexture> MTView::GetTexture() const
{
    if (texture_view_) {
        return texture_view_;
    } else if (resource_) {
        return resource_->GetTexture();
    }
    return {};
}

id<MTLAccelerationStructure> MTView::GetAccelerationStructure() const
{
    if (resource_) {
        return resource_->GetAccelerationStructure();
    }
    return {};
}

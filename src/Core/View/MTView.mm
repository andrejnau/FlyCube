#include "View/MTView.h"
#include <Device/MTDevice.h>
#include <Resource/MTResource.h>

static MTLTextureType ConvertTextureType(ViewDimension dimension)
{
    switch (dimension)
    {
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
#if TARGET_OS_OSX || TARGET_OS_IOS
    case ViewDimension::kTexture2DMSArray:
        return MTLTextureType2DMultisampleArray;
#endif
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

MTView::MTView(MTDevice& device, const std::shared_ptr<MTResource>& resource, const ViewDesc& m_view_desc)
    : m_device(device)
    , m_resource(resource)
    , m_view_desc(m_view_desc)
{
    if (!m_resource)
        return;
    switch (m_view_desc.view_type)
    {
    case ViewType::kTexture:
    case ViewType::kRWTexture:
        CreateTextureView();
        break;
    default:
        break;
    }
}

void MTView::CreateTextureView()
{
    decltype(auto) texture = m_resource->texture.res;
    MTLPixelFormat format = m_resource->texture.format;
    MTLTextureType texture_type = ConvertTextureType(m_view_desc.dimension);
    NSRange levels = {GetBaseMipLevel(), GetLevelCount()};
    NSRange slices = {GetBaseArrayLayer(), GetLayerCount()};
    MTLTextureSwizzleChannels swizzle = MTLTextureSwizzleChannelsDefault;
    
    if (m_view_desc.plane_slice == 1)
    {
        swizzle.green = MTLTextureSwizzleRed;
        if (format == MTLPixelFormatDepth32Float_Stencil8)
        {
            format = MTLPixelFormatX32_Stencil8;
        }
    }
        
    m_texture_view = [texture newTextureViewWithPixelFormat:format
                                                textureType:texture_type
                                                     levels:levels
                                                     slices:slices
                                                    swizzle:swizzle];
    if (m_texture_view == nullptr)
    {
        NSLog(@"Error: failed to create texture view");
    }
}

std::shared_ptr<Resource> MTView::GetResource()
{
    return m_resource;
}

uint32_t MTView::GetDescriptorId() const
{
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

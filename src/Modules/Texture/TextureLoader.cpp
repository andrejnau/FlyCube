#include "Texture/TextureLoader.h"
#include <Utilities/FormatHelper.h>
#include <gli/gli.hpp>

std::shared_ptr<Resource> CreateSRVFromFile(RenderDevice& device, RenderCommandList& command_list, const std::string& path)
{
    assert(false);
    return {};
}

std::shared_ptr<Resource> CreateSRVFromFileDDSKTX(RenderDevice& device, RenderCommandList& command_list, const std::string& path)
{
    gli::texture Texture = gli::load(path);
    auto format = Texture.format();
    uint32_t width = Texture.extent(0).x;
    uint32_t height = Texture.extent(0).y;
    size_t mip_levels = Texture.levels();

    std::shared_ptr<Resource> res = device.CreateTexture(BindFlag::kShaderResource | BindFlag::kCopyDest, format, 1, width, height, 1, mip_levels);

    for (std::size_t level = 0; level < mip_levels; ++level)
    {
        size_t row_bytes = 0;
        size_t num_bytes = 0;
        GetFormatInfo(Texture.extent(level).x, Texture.extent(level).y, format, num_bytes, row_bytes);
        command_list.UpdateSubresource(res, level, Texture.data(0, 0, level), row_bytes, num_bytes);
    }

    return res;
}

std::shared_ptr<Resource> CreateTexture(RenderDevice& device, RenderCommandList& command_list, const std::string& path)
{
    if (path.find(".dds") != -1 || path.find(".ktx") != -1)
        return CreateSRVFromFileDDSKTX(device, command_list, path);
    else
        return CreateSRVFromFile(device, command_list, path);
}

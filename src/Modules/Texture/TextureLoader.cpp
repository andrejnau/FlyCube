#include "Texture/TextureLoader.h"
#include "Texture/DXGIFormatHelper.h"
#include <Utilities/DXUtility.h>
#include <gli/gli.hpp>
#include <SOIL.h>

using namespace Microsoft::WRL;

Resource::Ptr CreateSRVFromFile(Context& context, const std::string& path)
{
    //return {};  // Generate MipMaps is not yet supported

    int width = 0;
    int height = 0;
    unsigned char* image = SOIL_load_image(path.c_str(), &width, &height, 0, SOIL_LOAD_RGBA);
    if (!image)
        return {};

    gli::format format = gli::format::FORMAT_RGBA8_UNORM_PACK8;
    Resource::Ptr res = context.CreateTexture(BindFlag::kSrv, format, 1, width, height);

    size_t num_bytes;
    size_t row_bytes;
    GetSurfaceInfo(width, height, format, &num_bytes, &row_bytes, nullptr);
    context.UpdateSubresource(res, 0, image, row_bytes, num_bytes);

    SOIL_free_image_data(image);

    return res;
}

Resource::Ptr CreateSRVFromFileDDS(Context& context, const std::string& path)
{
    gli::texture Texture = gli::load(path);
    auto format = Texture.format();
    uint32_t width = Texture.extent(0).x;
    uint32_t height = Texture.extent(0).y;
    size_t mip_levels = Texture.levels();

    Resource::Ptr res = context.CreateTexture(BindFlag::kSrv, format, 1, width, height, 1, mip_levels);

    for (std::size_t level = 0; level < mip_levels; ++level)
    {
        size_t num_bytes;
        size_t row_bytes;
        GetSurfaceInfo(Texture.extent(level).x, Texture.extent(level).y, format, &num_bytes, &row_bytes, nullptr);
        context.UpdateSubresource(res, level, Texture.data(0, 0, level), row_bytes, num_bytes);
    }

    return res;
}

Resource::Ptr CreateTexture(Context& context, const std::string& path)
{
    if (path.find(".dds") != -1)
        return CreateSRVFromFileDDS(context, path);
    else
        return CreateSRVFromFile(context, path);
}

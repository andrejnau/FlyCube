#pragma once

#include <string>

enum class TextureAssetsType
{
    kAlbedo,
    kNormal,
    kRoughness,
    kGlossiness,
    kMetalness,
    kOcclusion,
    kOpacity,
};

struct TextureInfo
{
    TextureAssetsType type;
    std::string path;
};

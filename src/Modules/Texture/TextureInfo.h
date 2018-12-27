#pragma once

#include <string>

enum class TextureType
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
    TextureType type;
    std::string path;
};

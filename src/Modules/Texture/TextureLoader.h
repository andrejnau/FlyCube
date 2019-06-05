#pragma once

#include "Texture/TextureInfo.h"
#include <Context/Context.h>

Resource::Ptr CreateTexture(Context& context, const std::string& path);

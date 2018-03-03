#pragma once

#include "Texture/TextureInfo.h"
#include <Context/Context.h>
#include <wrl.h>
using namespace Microsoft::WRL;

Resource::Ptr CreateTexture(Context& context, TextureInfo & texture);

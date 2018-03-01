#pragma once

#include "Texture/TextureInfo.h"
#include <Context/Context.h>
#include <wrl.h>
using namespace Microsoft::WRL;

ComPtr<IUnknown> CreateTexture(Context& context, TextureInfo & texture);

#pragma once

#include "Texture/TextureInfo.h"
#include <RenderDevice/RenderDevice.h>
#include <RenderCommandList/RenderCommandList.h>

std::shared_ptr<Resource> CreateTexture(RenderDevice& device, RenderCommandList& command_list, const std::string& path);

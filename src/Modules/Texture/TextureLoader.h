#pragma once

#include "Texture/TextureInfo.h"
#include <Device/Device.h>
#include <RenderCommandList/RenderCommandList.h>

std::shared_ptr<Resource> CreateTexture(Device& device, RenderCommandList& command_list, const std::string& path);

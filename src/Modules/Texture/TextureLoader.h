#pragma once

#include "Texture/TextureInfo.h"
#include <Context/Context.h>

std::shared_ptr<Resource> CreateTexture(Context& context, CommandListBox& command_list, const std::string& path);

#pragma once

#include "Texture/TextureInfo.h"
#include <Device/Device.h>
#include <CommandListBox/CommandListBox.h>

std::shared_ptr<Resource> CreateTexture(Device& device, CommandListBox& command_list, const std::string& path);

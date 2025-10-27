#pragma once

#include "Instance/BaseTypes.h"

#include <cstdint>
#include <string>
#include <vector>

bool AssetFileExists(const std::string& filepath);
std::vector<uint8_t> AssetLoadBinaryFile(const std::string& filepath);
std::vector<uint8_t> LoadShaderBlob(const std::string& filepath, ShaderBlobType blob_type);

#if defined(__ANDROID__)
struct AAssetManager;

void SetAAssetManager(AAssetManager* mgr);
#endif

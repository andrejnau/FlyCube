#pragma once

#include "Instance/BaseTypes.h"

#include <cstdint>
#include <string>
#include <vector>

uint64_t Align(uint64_t size, uint64_t alignment);
std::string GetAssertPath(const std::string& filepath);
std::vector<uint8_t> LoadShaderBlob(const std::string& filepath, ShaderBlobType blob_type);

#if defined(__ANDROID__)
struct AAssetManager;

void SetAAssetManager(AAssetManager* mgr);
#endif

template <typename T>
class PassKey {
private:
    friend T;
    PassKey() = default;
};

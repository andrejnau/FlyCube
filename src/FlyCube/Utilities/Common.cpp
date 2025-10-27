#include "Utilities/Common.h"

#include "Utilities/NotReached.h"
#include "Utilities/SystemUtils.h"

#include <filesystem>
#include <fstream>
#include <iterator>

#if defined(__APPLE__)
#import <Foundation/Foundation.h>
#elif defined(__ANDROID__)
#include <android/asset_manager.h>
#endif

namespace {

#if defined(__ANDROID__)
AAssetManager* g_asset_manager = nullptr;
#endif

std::filesystem::path GetUtf8Path(const std::string& path)
{
    std::u8string u8path(path.begin(), path.end());
    return std::filesystem::path(u8path);
}

std::string GetAssetPath(const std::string& filepath)
{
    if (!filepath.starts_with("assets")) {
        return filepath;
    }

#if defined(__APPLE__)
    NSBundle* main_bundle = [NSBundle mainBundle];
    if ([[main_bundle bundlePath] hasSuffix:@".app"]) {
        auto path = GetUtf8Path(filepath);
        auto resource = [main_bundle pathForResource:[NSString stringWithUTF8String:path.stem().c_str()]
                                              ofType:[NSString stringWithUTF8String:path.extension().c_str()]
                                         inDirectory:[NSString stringWithUTF8String:path.parent_path().c_str()]];
        if (resource) {
            return [resource UTF8String];
        }
        NOTREACHED();
    }
#elif defined(__ANDROID__)
    return filepath;
#endif
    return GetExecutableDir() + "/" + filepath;
}

} // namespace

uint64_t Align(uint64_t size, uint64_t alignment)
{
    return (size + (alignment - 1)) & ~(alignment - 1);
}

bool AssetFileExists(const std::string& filepath)
{
    std::string path = GetAssetPath(filepath);
#if defined(__ANDROID__)
    AAsset* file = AAssetManager_open(g_asset_manager, path.c_str(), AASSET_MODE_BUFFER);
    if (!file) {
        return false;
    }
    AAsset_close(file);
    return true;
#else
    return std::filesystem::exists(GetUtf8Path(path));
#endif
}

std::vector<uint8_t> AssetLoadBinaryFile(const std::string& filepath)
{
    std::string path = GetAssetPath(filepath);
#if defined(__ANDROID__)
    AAsset* file = AAssetManager_open(g_asset_manager, path.c_str(), AASSET_MODE_BUFFER);
    auto filesize = AAsset_getLength64(file);
    std::vector<uint8_t> data(filesize);
    AAsset_read(file, data.data(), filesize);
    AAsset_close(file);
    return data;
#else
    std::ifstream file(GetUtf8Path(path), std::ios::binary);
    file.unsetf(std::ios::skipws);

    std::streampos filesize;
    file.seekg(0, std::ios::end);
    filesize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data;
    data.reserve(filesize);
    data.insert(data.begin(), std::istream_iterator<uint8_t>(file), std::istream_iterator<uint8_t>());
    return data;
#endif
}

std::vector<uint8_t> LoadShaderBlob(const std::string& filepath, ShaderBlobType blob_type)
{
    std::string shader_blob_ext;
    if (blob_type == ShaderBlobType::kDXIL) {
        shader_blob_ext = ".dxil";
    } else {
        shader_blob_ext = ".spirv";
    }
    return AssetLoadBinaryFile(filepath + shader_blob_ext);
}

#if defined(__ANDROID__)
void SetAAssetManager(AAssetManager* mgr)
{
    g_asset_manager = mgr;
}
#endif

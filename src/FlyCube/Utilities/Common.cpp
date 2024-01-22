#include "Utilities/Common.h"

#include "Utilities/SystemUtils.h"

#include <filesystem>
#include <fstream>
#include <iterator>

#if defined(__APPLE__)
#import <Foundation/Foundation.h>
#endif

uint64_t Align(uint64_t size, uint64_t alignment)
{
    return (size + (alignment - 1)) & ~(alignment - 1);
}

std::vector<uint8_t> ReadBinaryFile(const std::string& filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    file.unsetf(std::ios::skipws);

    std::streampos filesize;
    file.seekg(0, std::ios::end);
    filesize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data;
    data.reserve(filesize);
    data.insert(data.begin(), std::istream_iterator<uint8_t>(file), std::istream_iterator<uint8_t>());
    return data;
}

std::string GetAssertPath(const std::string& filepath)
{
#if defined(__APPLE__)
    auto path = std::filesystem::path(filepath);
    auto resource = [[NSBundle mainBundle] pathForResource:[NSString stringWithUTF8String:path.stem().c_str()]
                                                    ofType:[NSString stringWithUTF8String:path.extension().c_str()]
                                               inDirectory:[NSString stringWithUTF8String:path.parent_path().c_str()]];
    if (resource) {
        return [resource UTF8String];
    }
#endif
    return GetExecutableDir() + "\\" + filepath;
}

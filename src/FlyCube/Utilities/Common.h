#pragma once
#include <cstdint>
#include <fstream>
#include <iterator>
#include <vector>

inline uint64_t Align(uint64_t size, uint64_t alignment)
{
    return (size + (alignment - 1)) & ~(alignment - 1);
}

inline std::vector<uint8_t> ReadBinaryFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
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

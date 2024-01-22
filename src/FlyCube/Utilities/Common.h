#pragma once

#include <cstdint>
#include <string>
#include <vector>

uint64_t Align(uint64_t size, uint64_t alignment);
std::vector<uint8_t> ReadBinaryFile(const std::string& filepath);
std::string GetAssertPath(const std::string& filepath);

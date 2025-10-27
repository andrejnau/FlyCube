#pragma once

#include <cstdint>

uint64_t Align(uint64_t size, uint64_t alignment);

template <typename T>
class PassKey {
private:
    friend T;
    PassKey() = default;
};

#pragma once

template <typename T>
class PassKey {
private:
    friend T;
    PassKey() = default;
};

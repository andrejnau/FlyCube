// http://blog.bitwigglers.org/using-enum-classes-as-type-safe-bitmasks/
#pragma once
#include <type_traits>

template <typename Enum>
struct EnableBitMaskOperators {
    static const bool enable = false;
};

#define ENABLE_BITMASK_OPERATORS(x)      \
    template <>                          \
    struct EnableBitMaskOperators<x> {   \
        static const bool enable = true; \
    };

template <typename Enum>
inline std::enable_if_t<EnableBitMaskOperators<Enum>::enable, Enum> operator|(Enum lhs, Enum rhs)
{
    return static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(lhs) |
                             static_cast<std::underlying_type_t<Enum>>(rhs));
}

template <typename Enum>
inline std::enable_if_t<EnableBitMaskOperators<Enum>::enable, Enum> operator&(Enum lhs, Enum rhs)
{
    return static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(lhs) &
                             static_cast<std::underlying_type_t<Enum>>(rhs));
}

template <typename Enum>
inline std::enable_if_t<EnableBitMaskOperators<Enum>::enable, Enum> operator^(Enum lhs, Enum rhs)
{
    return static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(lhs) ^
                             static_cast<std::underlying_type_t<Enum>>(rhs));
}

template <typename Enum>
inline std::enable_if_t<EnableBitMaskOperators<Enum>::enable, Enum> operator~(Enum rhs)
{
    return static_cast<Enum>(~static_cast<std::underlying_type_t<Enum>>(rhs));
}

template <typename Enum>
inline std::enable_if_t<EnableBitMaskOperators<Enum>::enable, Enum>& operator|=(Enum& lhs, Enum rhs)
{
    lhs = static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(lhs) |
                            static_cast<std::underlying_type_t<Enum>>(rhs));
    return lhs;
}

template <typename Enum>
inline std::enable_if_t<EnableBitMaskOperators<Enum>::enable, Enum>& operator&=(Enum& lhs, Enum rhs)
{
    lhs = static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(lhs) &
                            static_cast<std::underlying_type_t<Enum>>(rhs));
    return lhs;
}

template <typename Enum>
inline std::enable_if_t<EnableBitMaskOperators<Enum>::enable, Enum>& operator^=(Enum& lhs, Enum rhs)
{
    lhs = static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(lhs) ^
                            static_cast<std::underlying_type_t<Enum>>(rhs));
    return lhs;
}

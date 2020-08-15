#pragma once
// http://blog.bitwigglers.org/using-enum-classes-as-type-safe-bitmasks/

template<typename Enum>
struct EnableBitMaskOperators
{
    static const bool enable = false;
};

#define ENABLE_BITMASK_OPERATORS(x)  \
template<>                           \
struct EnableBitMaskOperators<x>     \
{                                    \
    static const bool enable = true; \
};

template<typename Enum>
inline typename std::enable_if_t<EnableBitMaskOperators<Enum>::enable, Enum> operator |(Enum lhs, Enum rhs)
{
    return static_cast<Enum>(
        static_cast<std::underlying_type<Enum>::type>(lhs) |
        static_cast<std::underlying_type<Enum>::type>(rhs)
        );
}

template<typename Enum>
inline typename std::enable_if_t<EnableBitMaskOperators<Enum>::enable, Enum> operator &(Enum lhs, Enum rhs)
{
    return static_cast<Enum> (
        static_cast<std::underlying_type<Enum>::type>(lhs) &
        static_cast<std::underlying_type<Enum>::type>(rhs)
        );
}

template<typename Enum>
inline typename std::enable_if_t<EnableBitMaskOperators<Enum>::enable, Enum> operator ^(Enum lhs, Enum rhs)
{
    return static_cast<Enum> (
        static_cast<std::underlying_type<Enum>::type>(lhs) ^
        static_cast<std::underlying_type<Enum>::type>(rhs)
        );
}

template<typename Enum>
inline typename std::enable_if_t<EnableBitMaskOperators<Enum>::enable, Enum> operator ~(Enum rhs)
{
    return static_cast<Enum> (
        ~static_cast<std::underlying_type<Enum>::type>(rhs)
        );
}

template<typename Enum>
inline typename std::enable_if_t<EnableBitMaskOperators<Enum>::enable, Enum>& operator |=(Enum& lhs, Enum rhs)
{
    lhs = static_cast<Enum> (
        static_cast<std::underlying_type<Enum>::type>(lhs) |
        static_cast<std::underlying_type<Enum>::type>(rhs)
        );
    return lhs;
}

template<typename Enum>
inline typename std::enable_if_t<EnableBitMaskOperators<Enum>::enable, Enum>& operator &=(Enum& lhs, Enum rhs)
{
    lhs = static_cast<Enum> (
        static_cast<std::underlying_type<Enum>::type>(lhs) &
        static_cast<std::underlying_type<Enum>::type>(rhs)
        );
    return lhs;
}

template<typename Enum>
inline typename std::enable_if_t<EnableBitMaskOperators<Enum>::enable, Enum>& operator ^=(Enum& lhs, Enum rhs)
{
    lhs = static_cast<Enum> (
        static_cast<std::underlying_type<Enum>::type>(lhs) ^
        static_cast<std::underlying_type<Enum>::type>(rhs)
        );
    return lhs;
}

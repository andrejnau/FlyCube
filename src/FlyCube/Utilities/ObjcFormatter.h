#pragma once

#import <Foundation/NSString.h>

#include <format>
#include <type_traits>

template <typename T>
    requires std::is_convertible_v<T, id>
struct std::formatter<T> {
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const T& obj, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", [[NSString stringWithFormat:@"%@", obj] UTF8String]);
    }
};

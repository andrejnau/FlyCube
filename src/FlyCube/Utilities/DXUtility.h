//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard
//

#pragma once

#include <cstdarg>
#include <cstdint>
#include <cstdio>

#ifdef _WIN32
#include <Windows.h>
#define DEBUGBREAK __debugbreak()
#else
#define DEBUGBREAK
#endif

#ifndef CUSTOM_FAILED
#define CUSTOM_FAILED FAILED
#endif

namespace DXUtility {
inline void Print(const char* msg)
{
    OutputDebugStringA(msg);
}
inline void Print(const wchar_t* msg)
{
    OutputDebugStringW(msg);
}

inline void Printf(const char* format, ...)
{
    char buffer[256];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buffer, 256, format, ap);
    Print(buffer);
}

inline void Printf(const wchar_t* format, ...)
{
    wchar_t buffer[256];
    va_list ap;
    va_start(ap, format);
    vswprintf(buffer, 256, format, ap);
    Print(buffer);
}

#ifndef NDEBUG
inline void PrintSubMessage(const char* format, ...)
{
    Print("--> ");
    char buffer[256];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buffer, 256, format, ap);
    Print(buffer);
    Print("\n");
}
inline void PrintSubMessage(const wchar_t* format, ...)
{
    Print("--> ");
    wchar_t buffer[256];
    va_list ap;
    va_start(ap, format);
    vswprintf(buffer, 256, format, ap);
    Print(buffer);
    Print("\n");
}
inline void PrintSubMessage(void) {}
#endif

} // namespace DXUtility

#ifdef ERROR
#undef ERROR
#endif
#ifdef ASSERT
#undef ASSERT
#endif
#ifdef HALT
#undef HALT
#endif

#define HALT(...) ERROR(__VA_ARGS__) DEBUGBREAK;

#ifdef NDEBUG

#define ASSERT(isTrue, ...) (void)(isTrue)
#define WARN_ONCE_IF(isTrue, ...) (void)(isTrue)
#define WARN_ONCE_IF_NOT(isTrue, ...) (void)(isTrue)
#define ERROR(msg, ...)
#define DEBUGPRINT(msg, ...) \
    do {                     \
    } while (0)
#define ASSERT_SUCCEEDED(hr, ...) (void)(hr)

#else // !RELEASE

#define STRINGIFY(x) #x
#define STRINGIFY_BUILTIN(x) STRINGIFY(x)
#define ASSERT(isFalse, ...)                                                                                           \
    if (!(bool)(isFalse)) {                                                                                            \
        DXUtility::Print("\nAssertion failed in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
        DXUtility::PrintSubMessage("\'" #isFalse "\' is false");                                                       \
        DXUtility::PrintSubMessage(__VA_ARGS__);                                                                       \
        DXUtility::Print("\n");                                                                                        \
        DEBUGBREAK;                                                                                                    \
    }

#define ASSERT_SUCCEEDED(expr, ...)                                                                         \
    {                                                                                                       \
        auto hr = expr;                                                                                     \
        if (CUSTOM_FAILED(hr)) {                                                                            \
            DXUtility::Print(                                                                               \
                "\nHRESULT failed in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
            DXUtility::PrintSubMessage("hr = 0x%08X", hr);                                                  \
            DXUtility::PrintSubMessage(__VA_ARGS__);                                                        \
            DXUtility::Print("\n");                                                                         \
            DEBUGBREAK;                                                                                     \
        }                                                                                                   \
    }

#define WARN_ONCE_IF(isTrue, ...)                                                                           \
    {                                                                                                       \
        static bool s_TriggeredWarning = false;                                                             \
        if ((bool)(isTrue) && !s_TriggeredWarning) {                                                        \
            s_TriggeredWarning = true;                                                                      \
            DXUtility::Print(                                                                               \
                "\nWarning issued in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
            DXUtility::PrintSubMessage("\'" #isTrue "\' is true");                                          \
            DXUtility::PrintSubMessage(__VA_ARGS__);                                                        \
            DXUtility::Print("\n");                                                                         \
        }                                                                                                   \
    }

#define WARN_ONCE_IF_NOT(isTrue, ...) WARN_ONCE_IF(!(isTrue), __VA_ARGS__)

#define ERROR(...)                                                                                               \
    DXUtility::Print("\nError reported in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
    DXUtility::PrintSubMessage(__VA_ARGS__);                                                                     \
    DXUtility::Print("\n");

#define DEBUGPRINT(msg, ...) DXUtility::Printf(msg "\n", ##__VA_ARGS__);

#endif

#define BreakIfFailed(hr)  \
    if (CUSTOM_FAILED(hr)) \
    DEBUGBREAK

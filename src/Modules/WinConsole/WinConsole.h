#pragma once

#include <Windows.h>

class WinConsole {
public:
    WinConsole()
        : m_in_stream(nullptr)
        , m_out_stream(nullptr)
        , m_err_stream(nullptr)
    {
        AllocConsole();
        freopen_s(&m_in_stream, "conin$", "r", stdin);
        freopen_s(&m_out_stream, "conout$", "w", stdout);
        freopen_s(&m_err_stream, "conout$", "w", stderr);
    }

    ~WinConsole()
    {
        FreeConsole();
    }

private:
    FILE* m_in_stream;
    FILE* m_out_stream;
    FILE* m_err_stream;
};

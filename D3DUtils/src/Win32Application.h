#pragma once

#include <Windows.h>
#include "IDXSample.h"
#include <string>

class Win32Application
{
public:
    static int Run(IDXSample* pSample, const std::wstring& title, HINSTANCE hInstance, int nCmdShow);
    static HWND GetHwnd();

protected:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    static HWND m_hwnd;
};

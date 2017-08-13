#pragma once

#include <Windows.h>
#include "IDXSample.h"

class Win32Application
{
public:
    static int Run(IDXSample* pSample, HINSTANCE hInstance, int nCmdShow);
    static HWND GetHwnd();

protected:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    static HWND m_hwnd;
};

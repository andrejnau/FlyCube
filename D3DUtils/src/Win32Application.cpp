#include "Win32Application.h"
#include <cstdio>

HWND Win32Application::m_hwnd = nullptr;

int Win32Application::Run(IDXSample* pSample, HINSTANCE hInstance, int nCmdShow)
{
    // Initialize the window class.
    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = L"DXSampleClass";
    RegisterClassEx(&windowClass);

    RECT windowRect = { 0, 0, static_cast<LONG>(pSample->GetWidth()), static_cast<LONG>(pSample->GetHeight()) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create the window and store a handle to it.
    m_hwnd = CreateWindow(
        windowClass.lpszClassName,
        L"[DX12] testApp",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,		// We have no parent window.
        nullptr,		// We aren't using menus.
        hInstance,
        pSample);

    // Initialize the sample. OnInit is defined in each child-implementation of DXSample.

    ShowWindow(m_hwnd, nCmdShow);
    pSample->OnInit();

    // Main sample loop.
    MSG msg = { 0 };
    while (true)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (msg.message == WM_QUIT)
            break;

        pSample->OnUpdate();
        pSample->OnRender();
    }

    pSample->OnDestroy();

    // Return this part of the WM_QUIT message to Windows.
    return static_cast<char>(msg.wParam);
}

HWND Win32Application::GetHwnd()
{
    return m_hwnd;
}

void RedirectIO()
{
    FILE *in_stream, *out_stream, *err_stream;
    freopen_s(&in_stream, "conin$", "r", stdin);
    freopen_s(&out_stream, "conout$", "w", stdout);
    freopen_s(&err_stream, "conout$", "w", stderr);
}

// Main message handler for the sample.
LRESULT CALLBACK Win32Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    IDXSample* pSample = reinterpret_cast<IDXSample*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message)
    {
        case WM_CREATE:
        {
            // Save the DXSample* passed in to CreateWindow.
            LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
#if defined(_DEBUG)
            AllocConsole();
            RedirectIO();
#endif
            break;
        }

        case WM_KEYDOWN:
        {
            if (wParam == VK_ESCAPE)
                 DestroyWindow(hWnd);
            break;
        }

        case WM_SIZE:
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            if (pSample && width > 0 && height > 0)
                pSample->OnSizeChanged(width, height);
            break;
        }

        case WM_DESTROY:
        {
#if defined(_DEBUG)
            FreeConsole();
#endif
            PostQuitMessage(0);
            break;
        }
    }

    // Handle any messages the switch statement didn't.
    return DefWindowProc(hWnd, message, wParam, lParam);
}
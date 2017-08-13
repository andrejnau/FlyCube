#include "Win32Application.h"
#include "DXSample.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    DXSample sample(1280, 720);
    return Win32Application::Run(&sample, L"[DX12] testApp", hInstance, nCmdShow);
}
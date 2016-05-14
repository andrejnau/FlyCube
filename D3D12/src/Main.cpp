#include "Win32Application.h"
#include "DXSample.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    DXSample sample(700, 700);
    return Win32Application::Run(&sample, hInstance, nCmdShow);
}
#pragma once

#include <d3d11.h>
#include <d3d11_1.h>
#include <DXGI1_4.h>
#include <wrl.h>
#include <GLFW/glfw3.h>

using namespace Microsoft::WRL;

class Context
{
public:
    Context(GLFWwindow* window, int width, int height);
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> device_context;
    ComPtr<IDXGISwapChain> swap_chain;
    ComPtr<ID3DUserDefinedAnnotation> perf;
    GLFWwindow* window;
};

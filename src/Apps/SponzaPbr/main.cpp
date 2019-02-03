#include "Scene.h"
#include <AppBox/AppBox.h>
#include <Utilities/State.h>
#include <WinConsole/WinConsole.h>

int main(int argc, char *argv[])
{
    WinConsole cmd; 
    ApiType type = ApiType::kVulkan;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == "--dx11")
            type = ApiType::kDX11;
        else if (arg == "--dx12")
            type = ApiType::kDX12;
        else if (arg == "--vk")
            type = ApiType::kVulkan;
        else if (arg == "--gl")
            type = ApiType::kOpenGL;
    }

    std::string title;
    switch (type)
    {
    case ApiType::kDX11:
        title = "[DX11] testApp";
        break;
    case ApiType::kDX12:
        title = "[DX12] testApp";
        break;
    case ApiType::kVulkan:
        title = "[Vulkan] testApp";
        break;
    case ApiType::kOpenGL:
        title = "[OpenGL] testApp";
        break;
    }

    auto monitor_desc = AppBox::GetPrimaryMonitorDesc();
    return AppBox(Scene::Create, type, title, monitor_desc.width / 1.5, monitor_desc.height / 1.5).Run();
}

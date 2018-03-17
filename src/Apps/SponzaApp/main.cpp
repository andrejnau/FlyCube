#include "DX11Scene.h"
#include <AppBox/AppBox.h>
#include <Utilities/State.h>

int main(int argc, char *argv[])
{
    ApiType type = ApiType::kDX11;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == "--dx11")
            type = ApiType::kDX11;
        else if (arg == "--dx12")
            type = ApiType::kDX12;
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
    }

    auto& state = CurState<bool>::Instance().state;
    state["DepthBias"] = true;
    if (0 && type == ApiType::kDX12 && LoadLibraryA("d3dcompiler_dxc_bridge.dll"))
    {
        state["DXIL"] = true;
        title += " with Shader Model 6.0";
    }

    return AppBox(DX11Scene::Create, type, title, 1280, 720).Run();
}

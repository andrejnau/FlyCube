#include "DX11Scene.h"
#include <AppBox/AppBox.h>
#include <Utilities/State.h>

int main(int argc, char *argv[])
{
    ApiType type = ApiType::kDX12;
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

    return AppBox(DX11Scene::Create, type, title, 1280, 720).Run();
}

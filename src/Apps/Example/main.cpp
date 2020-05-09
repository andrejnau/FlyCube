#include <AppBox/AppBox.h>
#include <Instance/Instance.h>

int main(int argc, char* argv[])
{
    AppBox app(argc, argv, "Example");

    std::unique_ptr<Instance> instance = CreateInstance(ApiType::kDX12);
    std::unique_ptr<Adapter> adapter = std::move(instance->EnumerateAdapters().front());
    std::unique_ptr<Device> device = adapter->CreateDevice();
    std::unique_ptr<Swapchain> swapchain = device->CreateSwapchain(app.GetWindow(), app.GetAppRect().width, app.GetAppRect().height, 3);
    std::shared_ptr<CommandList> command_list = device->CreateCommandList();

    while (!app.ShouldClose())
    {
        uint32_t frame_index = swapchain->GetCurrentBackBufferIndex();
        command_list->Open();
        command_list->Clear(swapchain->GetBackBuffer(frame_index), { 1, 0, 1, 0 });
        command_list->Close();
        device->ExecuteCommandLists({ command_list });
        swapchain->Present();
        app.PollEvents();
    }
    return 0;
}

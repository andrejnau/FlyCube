#include <AppBox/AppBox.h>
#include <Instance/Instance.h>

int main(int argc, char* argv[])
{
    AppBox app(argc, argv, "Example");

    std::unique_ptr<Instance> instance = CreateInstance(ApiType::kVulkan);
    std::unique_ptr<Adapter> adapter = std::move(instance->EnumerateAdapters().front());
    std::unique_ptr<Device> device = adapter->CreateDevice();
    std::unique_ptr<Swapchain> swapchain = device->CreateSwapchain(app.GetWindow(), app.GetAppRect().width, app.GetAppRect().height, 3);
    std::shared_ptr<CommandList> command_list = device->CreateCommandList();

    while (!app.ShouldClose())
    {
        uint32_t frame_index = swapchain->GetCurrentBackBufferIndex();
        Resource::Ptr back_buffer = swapchain->GetBackBuffer(frame_index);
        command_list->Open();
        command_list->ResourceBarrier(back_buffer, ResourceState::kCommon);
        command_list->Clear(back_buffer, { 1, 0, 1, 0 });
        command_list->ResourceBarrier(back_buffer, ResourceState::kPresent);
        command_list->Close();
        device->ExecuteCommandLists({ command_list });
        swapchain->Present();
        app.PollEvents();
    }
    return 0;
}

#include <AppBox/AppBox.h>
#include <Instance/Instance.h>

int main(int argc, char* argv[])
{
    AppBox app(argc, argv, "Example");

    std::unique_ptr<Instance> instance = CreateInstance(ApiType::kVulkan);
    std::unique_ptr<Adapter> adapter = std::move(instance->EnumerateAdapters().front());
    std::unique_ptr<Device> device = adapter->CreateDevice();
    std::unique_ptr<Swapchain> swapchain = device->CreateSwapchain(app.GetWindow(), app.GetAppRect().width, app.GetAppRect().height, 3);
    std::unique_ptr<CommandList> command_list = device->CreateCommandList();

    while (!app.ShouldClose())
    {
        app.PollEvents();
    }
    return 0;
}

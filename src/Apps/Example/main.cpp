#include <AppBox/AppBox.h>
#include <Instance/Instance.h>

int main(int argc, char* argv[])
{
    ApiType type = ApiType::kVulkan;
    AppBox app(argc, argv, "Example", type);
    std::unique_ptr<Instance> instance = CreateInstance(type);
    std::unique_ptr<Adapter> adapter = std::move(instance->EnumerateAdapters().front());
    std::unique_ptr<Device> device = adapter->CreateDevice();
    uint32_t frame_count = 3;
    std::unique_ptr<Swapchain> swapchain = device->CreateSwapchain(app.GetWindow(), app.GetAppRect().width, app.GetAppRect().height, frame_count);
    std::vector<std::shared_ptr<CommandList>> command_lists;
    std::vector<std::unique_ptr<Fence>> fences;
    for (size_t i = 0; i < frame_count; ++i)
    {
        command_lists.emplace_back(device->CreateCommandList());
        fences.emplace_back(device->CreateFence());
    }

    while (!app.ShouldClose())
    {
        uint32_t frame_index = swapchain->GetCurrentBackBufferIndex();
        Resource::Ptr back_buffer = swapchain->GetBackBuffer(frame_index);
        std::shared_ptr<CommandList> command_list = command_lists[frame_index];
        std::unique_ptr<Fence>& fence = fences[frame_index];
        fence->Wait();
        command_list->Open();
        command_list->ResourceBarrier(back_buffer, ResourceState::kClear);
        command_list->Clear(back_buffer, { 1, 0, 1, 0 });
        command_list->ResourceBarrier(back_buffer, ResourceState::kPresent);
        command_list->Close();
        fence->Reset();
        device->ExecuteCommandLists({ command_list }, fence);
        swapchain->Present();
        app.PollEvents();
    }
    return 0;
}

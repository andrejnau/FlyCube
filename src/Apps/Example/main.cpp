#include <AppBox/AppBox.h>
#include <Instance/Instance.h>

int main(int argc, char* argv[])
{
    ApiType type = ApiType::kVulkan;
    AppBox app(argc, argv, "Example", type);
    std::shared_ptr<Instance> instance = CreateInstance(type);
    std::shared_ptr<Adapter> adapter = std::move(instance->EnumerateAdapters().front());
    std::shared_ptr<Device> device = adapter->CreateDevice();
    uint32_t frame_count = 3;
    std::shared_ptr<Swapchain> swapchain = device->CreateSwapchain(app.GetWindow(), app.GetAppRect().width, app.GetAppRect().height, frame_count);
    std::vector<std::shared_ptr<CommandList>> command_lists;
    for (uint32_t i = 0; i < frame_count; ++i)
    {
        command_lists.emplace_back(device->CreateCommandList());
        std::shared_ptr<CommandList> command_list = command_lists[i];
        Resource::Ptr back_buffer = swapchain->GetBackBuffer(i);
        command_list->Open();
        command_list->ResourceBarrier(back_buffer, ResourceState::kClear);
        command_list->Clear(back_buffer, { 1, 0, 1, 0 });
        command_list->ResourceBarrier(back_buffer, ResourceState::kPresent);
        command_list->Close();
    }
    std::shared_ptr<Fence> fence = device->CreateFence();
    std::shared_ptr<Semaphore> image_available_semaphore = device->CreateGPUSemaphore();
    std::shared_ptr<Semaphore> rendering_finished_semaphore = device->CreateGPUSemaphore();
    while (!app.ShouldClose())
    {
        fence->WaitAndReset();
        uint32_t frame_index = swapchain->NextImage(image_available_semaphore);
        device->ExecuteCommandLists({ command_lists[frame_index] }, fence, { image_available_semaphore }, { rendering_finished_semaphore });
        swapchain->Present(rendering_finished_semaphore);
        app.PollEvents();
        app.UpdateFps();
    }
    _exit(0);
}

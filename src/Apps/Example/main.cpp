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
    std::vector<std::shared_ptr<Fence>> fences;
    std::shared_ptr<Semaphore> image_available_semaphore = device->CreateGPUSemaphore();
    std::shared_ptr<Semaphore> rendering_finished_semaphore = device->CreateGPUSemaphore();
    for (size_t i = 0; i < frame_count; ++i)
    {
        command_lists.emplace_back(device->CreateCommandList());
        fences.emplace_back(device->CreateFence());
    }

    while (!app.ShouldClose())
    {
        uint32_t frame_index = swapchain->NextImage(image_available_semaphore);
        Resource::Ptr back_buffer = swapchain->GetBackBuffer(frame_index);
        std::shared_ptr<CommandList> command_list = command_lists[frame_index];
        std::shared_ptr<Fence>& fence = fences[frame_index];
        fence->Wait();
        command_list->Open();
        command_list->ResourceBarrier(back_buffer, ResourceState::kClear);
        command_list->Clear(back_buffer, { 1, 0, 1, 0 });
        command_list->ResourceBarrier(back_buffer, ResourceState::kPresent);
        command_list->Close();
        fence->Reset();
        device->ExecuteCommandLists({ command_list }, fence, { image_available_semaphore }, { rendering_finished_semaphore });
        swapchain->Present(rendering_finished_semaphore);
        app.PollEvents();
        app.UpdateFps();
    }
    _exit(0);
}

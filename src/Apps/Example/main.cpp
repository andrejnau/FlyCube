#include <AppBox/AppBox.h>
#include <Instance/Instance.h>
#include <Utilities/FileUtility.h>

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
    std::vector<std::shared_ptr<View>> views;

    std::shared_ptr<Shader> vertex_shader = device->CompileShader({ "shaders/Triangle/VertexShader_VS.hlsl", "main", ShaderType::kVertex });
    std::shared_ptr<Shader> pixel_shader = device->CompileShader({ "shaders/Triangle/PixelShader_PS.hlsl", "main",  ShaderType::kPixel });

    std::vector<uint32_t> index_data = { 0, 1, 2 };
    std::shared_ptr<Resource> index_buffer = device->CreateBuffer(BindFlag::kIbv, sizeof(uint32_t) * index_data.size(), sizeof(uint32_t));

    std::vector<glm::vec3> vertex_data = { glm::vec3(-0.5, -0.5, 0.0), glm::vec3(0.0,  0.5, 0.0), glm::vec3(0.5, -0.5, 0.0) };
    std::shared_ptr<Resource> vertex_buffer = device->CreateBuffer(BindFlag::kVbv, sizeof(glm::vec3) * vertex_data.size(), sizeof(glm::vec3));

    for (uint32_t i = 0; i < frame_count; ++i)
    {
        ViewDesc view_desc = {};
        view_desc.res_type = ResourceType::kRtv;
        std::shared_ptr<Resource> back_buffer = swapchain->GetBackBuffer(i);
        std::shared_ptr<View> back_buffer_view = device->CreateView(back_buffer, view_desc);
        command_lists.emplace_back(device->CreateCommandList());
        std::shared_ptr<CommandList> command_list = command_lists[i];
        command_list->Open();
        command_list->ResourceBarrier(back_buffer, ResourceState::kClear);
        command_list->Clear(back_buffer_view, { 1, 0, 1, 0 });
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
        device->Wait(image_available_semaphore);
        device->ExecuteCommandLists({ command_lists[frame_index] }, fence);
        device->Signal(rendering_finished_semaphore);
        swapchain->Present(rendering_finished_semaphore);
        app.PollEvents();
        app.UpdateFps();
    }
    _exit(0);
}

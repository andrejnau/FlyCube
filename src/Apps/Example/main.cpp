#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <Instance/Instance.h>

int main(int argc, char* argv[])
{
    Settings settings = { ApiType::kDX12 };
    ParseArgs(argc, argv, settings);
    AppBox app("Example", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<Instance> instance = CreateInstance(settings.api_type);
    std::shared_ptr<Adapter> adapter = std::move(instance->EnumerateAdapters()[settings.required_gpu_index]);
    std::shared_ptr<Device> device = adapter->CreateDevice();

    uint32_t frame_count = 3;
    std::shared_ptr<Swapchain> swapchain = device->CreateSwapchain(app.GetWindow(), rect.width, rect.height, frame_count, settings.vsync);

    std::vector<uint32_t> index_data = { 0, 1, 2 };
    std::shared_ptr<Resource> index_buffer = device->CreateBuffer(BindFlag::kIbv, sizeof(uint32_t) * index_data.size(), sizeof(uint32_t));
    std::vector<glm::vec3> vertex_data = { glm::vec3(-0.5, -0.5, 0.0), glm::vec3(0.0,  0.5, 0.0), glm::vec3(0.5, -0.5, 0.0) };
    std::shared_ptr<Resource> vertex_buffer = device->CreateBuffer(BindFlag::kVbv, sizeof(vertex_data.front()) * vertex_data.size(), sizeof(vertex_data.front()));
    glm::vec4 constant_data = glm::vec4(1, 0, 0, 1);
    std::shared_ptr<Resource> constant_buffer = device->CreateBuffer(BindFlag::kCbv, sizeof(constant_data), 0);

    std::shared_ptr<CommandList> upload_command_list = device->CreateCommandList();
    upload_command_list->Open();
    upload_command_list->UpdateSubresource(index_buffer, 0, index_data.data());
    upload_command_list->UpdateSubresource(vertex_buffer, 0, vertex_data.data());
    upload_command_list->UpdateSubresource(constant_buffer, 0, &constant_data);
    upload_command_list->Close();
    device->ExecuteCommandLists({ upload_command_list });

    ViewDesc constant_view_desc = {};
    constant_view_desc.res_type = ResourceType::kCbv;
    std::shared_ptr<View> constant_buffer_view = device->CreateView(constant_buffer, constant_view_desc);
    std::shared_ptr<Shader> vertex_shader = device->CompileShader({ "shaders/Triangle/VertexShader_VS.hlsl", "main", ShaderType::kVertex });
    std::shared_ptr<Shader> pixel_shader = device->CompileShader({ "shaders/Triangle/PixelShader_PS.hlsl", "main",  ShaderType::kPixel });
    std::shared_ptr<Program> program = device->CreateProgram({ vertex_shader, pixel_shader });
    std::shared_ptr<BindingSet> binding_set = program->CreateBindingSet({ { ShaderType::kPixel, ResourceType::kCbv, "Settings", constant_buffer_view } });
    GraphicsPipelineDesc pipeline_desc = {
        program,
        { { 0, "POSITION", gli::FORMAT_RGB32_SFLOAT_PACK32 } },
        { { 0, swapchain->GetFormat() } },
    };
    std::shared_ptr<Pipeline> pipeline = device->CreateGraphicsPipeline(pipeline_desc);

    std::vector<std::shared_ptr<CommandList>> command_lists;
    std::vector<std::shared_ptr<View>> views;
    std::vector<std::shared_ptr<Framebuffer>> framebuffers;
    for (uint32_t i = 0; i < frame_count; ++i)
    {
        std::shared_ptr<Resource> back_buffer = swapchain->GetBackBuffer(i);
        ViewDesc back_buffer_view_desc = {};
        back_buffer_view_desc.res_type = ResourceType::kRtv;
        std::shared_ptr<View> back_buffer_view = device->CreateView(back_buffer, back_buffer_view_desc);
        framebuffers.emplace_back(device->CreateFramebuffer(pipeline, { back_buffer_view }));
        command_lists.emplace_back(device->CreateCommandList());
        std::shared_ptr<CommandList> command_list = command_lists[i];
        command_list->Open();
        command_list->ResourceBarrier(back_buffer, ResourceState::kClear);
        command_list->Clear(back_buffer_view, { 0.0, 0.2, 0.4, 1.0 });
        command_list->ResourceBarrier(back_buffer, ResourceState::kRenderTarget);
        command_list->BindPipeline(pipeline);
        command_list->BindBindingSet(binding_set);
        command_list->BeginRenderPass(framebuffers.back());
        command_list->SetViewport(rect.width, rect.height);
        command_list->IASetIndexBuffer(index_buffer, gli::format::FORMAT_R32_UINT_PACK32);
        command_list->IASetVertexBuffer(0, vertex_buffer);
        command_list->DrawIndexed(3, 0, 0);
        command_list->EndRenderPass();
        command_list->ResourceBarrier(back_buffer, ResourceState::kPresent);
        command_list->Close();
    }

    std::shared_ptr<Fence> fence = device->CreateFence();
    std::shared_ptr<Semaphore> image_available_semaphore = device->CreateGPUSemaphore();
    std::shared_ptr<Semaphore> rendering_finished_semaphore = device->CreateGPUSemaphore();

    while (!app.PollEvents())
    {
        fence->WaitAndReset();
        uint32_t frame_index = swapchain->NextImage(image_available_semaphore);
        device->Wait(image_available_semaphore);
        device->ExecuteCommandLists({ command_lists[frame_index] }, fence);
        device->Signal(rendering_finished_semaphore);
        swapchain->Present(rendering_finished_semaphore);
        app.UpdateFps();
    }
    _exit(0);
}

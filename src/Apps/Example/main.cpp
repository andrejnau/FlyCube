#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <Instance/Instance.h>

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("Example", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<Instance> instance = CreateInstance(settings.api_type);
    std::shared_ptr<Adapter> adapter = std::move(instance->EnumerateAdapters()[settings.required_gpu_index]);
    std::shared_ptr<Device> device = adapter->CreateDevice();

    uint32_t frame_count = 3;
    std::shared_ptr<Swapchain> swapchain = device->CreateSwapchain(app.GetWindow(), rect.width, rect.height, frame_count, settings.vsync);

    std::vector<uint32_t> index_data = { 0, 1, 2 };
    std::shared_ptr<Resource> index_buffer = device->CreateBuffer(BindFlag::kIndexBuffer, sizeof(uint32_t) * index_data.size(), MemoryType::kUpload);
    index_buffer->UpdateUploadData(index_data.data(), 0, sizeof(index_data.front()) * index_data.size());

    std::vector<glm::vec3> vertex_data = { glm::vec3(-0.5, -0.5, 0.0), glm::vec3(0.0,  0.5, 0.0), glm::vec3(0.5, -0.5, 0.0) };
    std::shared_ptr<Resource> vertex_buffer = device->CreateBuffer(BindFlag::kVertexBuffer, sizeof(vertex_data.front()) * vertex_data.size(), MemoryType::kUpload);
    vertex_buffer->UpdateUploadData(vertex_data.data(), 0, sizeof(vertex_data.front()) * vertex_data.size());

    glm::vec4 constant_data = glm::vec4(1, 0, 0, 1);
    std::shared_ptr<Resource> constant_buffer = device->CreateBuffer(BindFlag::kConstantBuffer, sizeof(constant_data), MemoryType::kUpload);
    constant_buffer->UpdateUploadData(&constant_data, 0, sizeof(constant_data));

    ViewDesc constant_view_desc = {};
    constant_view_desc.view_type = ViewType::kConstantBuffer;
    std::shared_ptr<View> constant_buffer_view = device->CreateView(constant_buffer, constant_view_desc);
    std::shared_ptr<Shader> vertex_shader = device->CompileShader({ "shaders/Triangle/VertexShader_VS.hlsl", "main", ShaderType::kVertex });
    std::shared_ptr<Shader> pixel_shader = device->CompileShader({ "shaders/Triangle/PixelShader_PS.hlsl", "main",  ShaderType::kPixel });
    std::shared_ptr<Program> program = device->CreateProgram({ vertex_shader, pixel_shader });
    std::shared_ptr<BindingSet> binding_set = program->CreateBindingSet({ { pixel_shader->GetBindKey("Settings"), constant_buffer_view } });
    GraphicsPipelineDesc pipeline_desc = {
        program,
        { { 0, "POSITION", gli::FORMAT_RGB32_SFLOAT_PACK32, sizeof(vertex_data.front()) } },
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
        back_buffer_view_desc.view_type = ViewType::kRenderTarget;
        std::shared_ptr<View> back_buffer_view = device->CreateView(back_buffer, back_buffer_view_desc);
        framebuffers.emplace_back(device->CreateFramebuffer(pipeline, rect.width, rect.height, { back_buffer_view }));
        command_lists.emplace_back(device->CreateCommandList());
        std::shared_ptr<CommandList> command_list = command_lists[i];
        command_list->Open();
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kClearColor} });
        command_list->ClearColor(back_buffer_view, { 0.0, 0.2, 0.4, 1.0 });
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kClearColor, ResourceState::kRenderTarget} });
        command_list->BindPipeline(pipeline);
        command_list->BindBindingSet(binding_set);
        command_list->BeginRenderPass(framebuffers.back());
        command_list->SetViewport(rect.width, rect.height);
        command_list->IASetIndexBuffer(index_buffer, gli::format::FORMAT_R32_UINT_PACK32);
        command_list->IASetVertexBuffer(0, vertex_buffer);
        command_list->DrawIndexed(3, 0, 0);
        command_list->EndRenderPass();
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kRenderTarget, ResourceState::kPresent} });
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
    std::shared_ptr<Fence> idle_fence = device->CreateFence(FenceFlag::kNone);
    device->Signal(idle_fence);
    idle_fence->WaitAndReset();
    return 0;
}

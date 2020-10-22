#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <Instance/Instance.h>

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("CoreTriangle", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<Instance> instance = CreateInstance(settings.api_type);
    std::shared_ptr<Adapter> adapter = std::move(instance->EnumerateAdapters()[settings.required_gpu_index]);
    app.SetGpuName(adapter->GetName());
    std::shared_ptr<Device> device = adapter->CreateDevice();
    std::shared_ptr<CommandQueue> command_queue = device->GetCommandQueue(CommandListType::kGraphics);
    std::shared_ptr<CommandQueue> copy_command_queue = device->GetCommandQueue(CommandListType::kCopy);
    constexpr uint32_t frame_count = 3;
    std::shared_ptr<Swapchain> swapchain = device->CreateSwapchain(app.GetWindow(), rect.width, rect.height, frame_count, settings.vsync);
    uint64_t fence_value = 0;
    std::shared_ptr<Fence> fence = device->CreateFence(fence_value);

    std::vector<uint32_t> index_data = { 0, 1, 2 };
    std::shared_ptr<Resource> index_buffer = device->CreateBuffer(BindFlag::kIndexBuffer | BindFlag::kCopyDest, sizeof(uint32_t) * index_data.size());
    index_buffer->CommitMemory(MemoryType::kDefault);
    std::vector<glm::vec3> vertex_data = { glm::vec3(-0.5, -0.5, 0.0), glm::vec3(0.0,  0.5, 0.0), glm::vec3(0.5, -0.5, 0.0) };
    std::shared_ptr<Resource> vertex_buffer = device->CreateBuffer(BindFlag::kVertexBuffer | BindFlag::kCopyDest, sizeof(vertex_data.front()) * vertex_data.size());
    vertex_buffer->CommitMemory(MemoryType::kDefault);
    glm::vec4 constant_data = glm::vec4(1, 0, 0, 1);
    std::shared_ptr<Resource> constant_buffer = device->CreateBuffer(BindFlag::kConstantBuffer | BindFlag::kCopyDest, sizeof(constant_data));
    constant_buffer->CommitMemory(MemoryType::kDefault);

    std::shared_ptr<Resource> upload_buffer = device->CreateBuffer(BindFlag::kCopySource, index_buffer->GetWidth() + vertex_buffer->GetWidth() + constant_buffer->GetWidth());
    auto mem_requirements = upload_buffer->GetMemoryRequirements();
    std::shared_ptr<Memory> memory = device->AllocateMemory(mem_requirements.size, MemoryType::kUpload, mem_requirements.memory_type_bits);
    upload_buffer->BindMemory(memory, 0);
    upload_buffer->UpdateUploadBuffer(0, index_data.data(), sizeof(index_data.front()) * index_data.size());
    upload_buffer->UpdateUploadBuffer(index_buffer->GetWidth(), vertex_data.data(), sizeof(vertex_data.front()) * vertex_data.size());
    upload_buffer->UpdateUploadBuffer(index_buffer->GetWidth() + vertex_buffer->GetWidth(), &constant_data, sizeof(constant_data));

    std::shared_ptr<CommandList> upload_command_list = device->CreateCommandList(CommandListType::kCopy);
    upload_command_list->CopyBuffer(upload_buffer, index_buffer, { { 0, 0, index_buffer->GetWidth() } });
    upload_command_list->CopyBuffer(upload_buffer, vertex_buffer, { { index_buffer->GetWidth(), 0, vertex_buffer->GetWidth() } });
    upload_command_list->CopyBuffer(upload_buffer, constant_buffer, { { index_buffer->GetWidth() + vertex_buffer->GetWidth(), 0, constant_buffer->GetWidth() } });
    upload_command_list->Close();

    copy_command_queue->ExecuteCommandLists({ upload_command_list });
    copy_command_queue->Signal(fence, ++fence_value);
    command_queue->Wait(fence, fence_value);

    ViewDesc constant_view_desc = {};
    constant_view_desc.view_type = ViewType::kConstantBuffer;
    std::shared_ptr<View> constant_buffer_view = device->CreateView(constant_buffer, constant_view_desc);
    std::shared_ptr<Shader> vertex_shader = device->CompileShader({ "shaders/Triangle/VertexShader_VS.hlsl", "main", ShaderType::kVertex, "6_0" });
    std::shared_ptr<Shader> pixel_shader = device->CompileShader({ "shaders/Triangle/PixelShader_PS.hlsl", "main",  ShaderType::kPixel, "6_0" });
    std::shared_ptr<Program> program = device->CreateProgram({ vertex_shader, pixel_shader });
    std::shared_ptr<BindingSet> binding_set = program->CreateBindingSet({ { pixel_shader->GetBindKey("Settings"), constant_buffer_view } });
    RenderPassDesc render_pass_desc = {
        { { swapchain->GetFormat(), RenderPassLoadOp::kClear, RenderPassStoreOp::kStore } },
    };
    std::shared_ptr<RenderPass> render_pass = device->CreateRenderPass(render_pass_desc);
    ClearDesc clear_desc = { { { 0.0, 0.2, 0.4, 1.0 } } };
    GraphicsPipelineDesc pipeline_desc = {
        program,
        { { 0, "POSITION", gli::FORMAT_RGB32_SFLOAT_PACK32, sizeof(vertex_data.front()) } },
        render_pass
    };
    std::shared_ptr<Pipeline> pipeline = device->CreateGraphicsPipeline(pipeline_desc);

    std::array<uint64_t, frame_count> fence_values = {};
    std::vector<std::shared_ptr<CommandList>> command_lists;
    std::vector<std::shared_ptr<Framebuffer>> framebuffers;
    for (uint32_t i = 0; i < frame_count; ++i)
    {
        std::shared_ptr<Resource> back_buffer = swapchain->GetBackBuffer(i);
        ViewDesc back_buffer_view_desc = {};
        back_buffer_view_desc.view_type = ViewType::kRenderTarget;
        std::shared_ptr<View> back_buffer_view = device->CreateView(back_buffer, back_buffer_view_desc);
        framebuffers.emplace_back(device->CreateFramebuffer(render_pass, rect.width, rect.height, { back_buffer_view }));
        command_lists.emplace_back(device->CreateCommandList(CommandListType::kGraphics));
        std::shared_ptr<CommandList> command_list = command_lists[i];
        command_list->BindPipeline(pipeline);
        command_list->BindBindingSet(binding_set);
        command_list->SetViewport(0, 0, rect.width, rect.height);
        command_list->SetScissorRect(0, 0, rect.width, rect.height);
        command_list->IASetIndexBuffer(index_buffer, gli::format::FORMAT_R32_UINT_PACK32);
        command_list->IASetVertexBuffer(0, vertex_buffer);
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kRenderTarget } });
        command_list->BeginRenderPass(render_pass, framebuffers.back(), clear_desc);
        command_list->DrawIndexed(3, 0, 0);
        command_list->EndRenderPass();
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kRenderTarget, ResourceState::kPresent } });
        command_list->Close();
    }

    while (!app.PollEvents())
    {
        uint32_t frame_index = swapchain->NextImage(fence, ++fence_value);
        command_queue->Wait(fence, fence_value);
        fence->Wait(fence_values[frame_index]);
        command_queue->ExecuteCommandLists({ command_lists[frame_index] });
        command_queue->Signal(fence, fence_values[frame_index] = ++fence_value);
        swapchain->Present(fence, fence_values[frame_index]);
    }
    command_queue->Signal(fence, ++fence_value);
    fence->Wait(fence_value);
    return 0;
}

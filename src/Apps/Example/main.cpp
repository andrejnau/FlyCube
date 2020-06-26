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

    constexpr uint32_t frame_count = 3;
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
    std::shared_ptr<Shader> vertex_shader = device->CompileShader({ "shaders/Triangle/VertexShader_VS.hlsl", "main", ShaderType::kVertex, "6_0" });
    std::shared_ptr<Shader> pixel_shader = device->CompileShader({ "shaders/Triangle/PixelShader_PS.hlsl", "main",  ShaderType::kPixel, "6_0" });
    std::shared_ptr<Program> program = device->CreateProgram({ vertex_shader, pixel_shader });
    std::shared_ptr<BindingSet> binding_set = program->CreateBindingSet({ { pixel_shader->GetBindKey("Settings"), constant_buffer_view } });
    RenderPassDesc render_pass_desc = {
        { { swapchain->GetFormat(), RenderPassLoadOp::kClear, RenderPassStoreOp::kStore } },
    };
    std::shared_ptr<RenderPass> render_pass = device->CreateRenderPass(render_pass_desc);
    GraphicsPipelineDesc pipeline_desc = {
        program,
        { { 0, "POSITION", gli::FORMAT_RGB32_SFLOAT_PACK32, sizeof(vertex_data.front()) } },
        render_pass
    };
    std::shared_ptr<Pipeline> pipeline = device->CreateGraphicsPipeline(pipeline_desc);

    std::vector<std::shared_ptr<CommandList>> command_lists;
    std::vector<std::shared_ptr<View>> views;
    std::vector<std::shared_ptr<Framebuffer>> framebuffers;
    for (uint32_t i = 0; i < frame_count; ++i)
    {
        framebuffers.emplace_back(device->CreateFramebuffer(render_pass, rect.width, rect.height, { swapchain->GetBackBufferView(i) }));
        command_lists.emplace_back(device->CreateCommandList());
        std::shared_ptr<CommandList> command_list = command_lists[i];
        command_list->BindPipeline(pipeline);
        command_list->BindBindingSet(binding_set);
        command_list->SetViewport(rect.width, rect.height);
        command_list->IASetIndexBuffer(index_buffer, gli::format::FORMAT_R32_UINT_PACK32);
        command_list->IASetVertexBuffer(0, vertex_buffer);
        command_list->ResourceBarrier({ { swapchain->GetBackBuffer(i), ResourceState::kRenderTarget } });
        command_list->BeginRenderPass(render_pass, framebuffers.back(), { { 0.0, 0.2, 0.4, 1.0 } });
        command_list->DrawIndexed(3, 0, 0);
        command_list->EndRenderPass();
        command_list->ResourceBarrier({ { swapchain->GetBackBuffer(i), ResourceState::kPresent } });
        command_list->Close();
    }

    std::array<uint64_t, frame_count> fence_values = {};
    uint64_t fence_value = 0;
    std::shared_ptr<Fence> fence = device->CreateFence(fence_value);

    while (!app.PollEvents())
    {
        uint32_t frame_index = swapchain->NextImage(fence, ++fence_value);
        device->Wait(fence, fence_value);
        fence->Wait(fence_values[frame_index]);
        device->ExecuteCommandLists({ command_lists[frame_index] });
        device->Signal(fence, fence_values[frame_index] = ++fence_value);
        swapchain->Present(fence, fence_values[frame_index]);
        app.UpdateFps(adapter->GetName());
    }
    device->Signal(fence, ++fence_value);
    fence->Wait(fence_value);
    return 0;
}

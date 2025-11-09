#include "AppBox/AppBox.h"
#include "AppSettings/ArgsParser.h"
#include "Instance/Instance.h"

namespace {

constexpr uint32_t kFrameCount = 3;

} // namespace

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("DesktopTriangle", settings);
    AppSize app_size = app.GetAppSize();

    std::shared_ptr<Instance> instance = CreateInstance(settings.api_type);
    std::shared_ptr<Adapter> adapter = std::move(instance->EnumerateAdapters()[settings.required_gpu_index]);
    app.SetGpuName(adapter->GetName());
    std::shared_ptr<Device> device = adapter->CreateDevice();
    std::shared_ptr<CommandQueue> command_queue = device->GetCommandQueue(CommandListType::kGraphics);
    std::shared_ptr<Swapchain> swapchain = device->CreateSwapchain(app.GetNativeWindow(), app_size.width(),
                                                                   app_size.height(), kFrameCount, settings.vsync);
    uint64_t fence_value = 0;
    std::shared_ptr<Fence> fence = device->CreateFence(fence_value);

    std::vector<uint32_t> index_data = { 0, 1, 2 };
    std::shared_ptr<Resource> index_buffer = device->CreateBuffer(
        MemoryType::kUpload,
        { .size = sizeof(index_data.front()) * index_data.size(), .usage = BindFlag::kIndexBuffer });
    index_buffer->UpdateUploadBuffer(0, index_data.data(), sizeof(index_data.front()) * index_data.size());

    std::vector<glm::vec3> vertex_data = {
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3(0.0, 0.5, 0.0),
        glm::vec3(0.5, -0.5, 0.0),
    };
    std::shared_ptr<Resource> vertex_buffer = device->CreateBuffer(
        MemoryType::kUpload,
        { .size = sizeof(vertex_data.front()) * vertex_data.size(), .usage = BindFlag::kVertexBuffer });
    vertex_buffer->UpdateUploadBuffer(0, vertex_data.data(), sizeof(vertex_data.front()) * vertex_data.size());

    glm::vec4 constant_data = glm::vec4(1.0, 0.0, 0.0, 1.0);
    std::shared_ptr<Resource> constant_buffer = device->CreateBuffer(
        MemoryType::kUpload, { .size = sizeof(constant_data), .usage = BindFlag::kConstantBuffer });
    constant_buffer->UpdateUploadBuffer(0, &constant_data, sizeof(constant_data));
    ViewDesc constant_buffer_view_desc = {
        .view_type = ViewType::kConstantBuffer,
        .dimension = ViewDimension::kBuffer,
    };
    std::shared_ptr<View> constant_buffer_view = device->CreateView(constant_buffer, constant_buffer_view_desc);

    std::shared_ptr<Shader> vertex_shader =
        device->CompileShader({ ASSETS_PATH "shaders/Triangle/VertexShader.hlsl", "main", ShaderType::kVertex, "6_0" });
    std::shared_ptr<Shader> pixel_shader =
        device->CompileShader({ ASSETS_PATH "shaders/Triangle/PixelShader.hlsl", "main", ShaderType::kPixel, "6_0" });

    BindKey constant_buffer_key = {
        .shader_type = ShaderType::kPixel,
        .view_type = ViewType::kConstantBuffer,
        .slot = 0,
        .space = 0,
        .count = 1,
    };
    std::shared_ptr<BindingSetLayout> layout = device->CreateBindingSetLayout({ constant_buffer_key });
    std::shared_ptr<BindingSet> binding_set = device->CreateBindingSet(layout);
    binding_set->WriteBindings({ { constant_buffer_key, constant_buffer_view } });

    RenderPassDesc render_pass_desc = {
        { { swapchain->GetFormat(), RenderPassLoadOp::kClear, RenderPassStoreOp::kStore } },
    };
    std::shared_ptr<RenderPass> render_pass = device->CreateRenderPass(render_pass_desc);
    GraphicsPipelineDesc pipeline_desc = {
        device->CreateProgram({ vertex_shader, pixel_shader }),
        layout,
        { { 0, "POSITION", gli::FORMAT_RGB32_SFLOAT_PACK32, sizeof(vertex_data.front()) } },
        render_pass,
    };
    std::shared_ptr<Pipeline> pipeline = device->CreateGraphicsPipeline(pipeline_desc);

    std::array<std::shared_ptr<View>, kFrameCount> back_buffer_views = {};
    std::array<std::shared_ptr<CommandList>, kFrameCount> command_lists = {};
    std::array<uint64_t, kFrameCount> fence_values = {};
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        std::shared_ptr<Resource> back_buffer = swapchain->GetBackBuffer(i);
        ViewDesc back_buffer_view_desc = {
            .view_type = ViewType::kRenderTarget,
            .dimension = ViewDimension::kTexture2D,
        };
        back_buffer_views[i] = device->CreateView(back_buffer, back_buffer_view_desc);

        FramebufferDesc framebuffer_desc = {
            .width = app_size.width(),
            .height = app_size.height(),
            .colors = { back_buffer_views[i] },
        };

        auto& command_list = command_lists[i];
        command_list = device->CreateCommandList(CommandListType::kGraphics);
        command_list->BindPipeline(pipeline);
        command_list->BindBindingSet(binding_set);
        command_list->SetViewport(0, 0, app_size.width(), app_size.height());
        command_list->SetScissorRect(0, 0, app_size.width(), app_size.height());
        command_list->IASetIndexBuffer(index_buffer, 0, gli::format::FORMAT_R32_UINT_PACK32);
        command_list->IASetVertexBuffer(0, vertex_buffer, 0);
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kRenderTarget } });
        ClearDesc clear_desc = { { { 0.0, 0.2, 0.4, 1.0 } } };
        command_list->BeginRenderPass(render_pass, framebuffer_desc, clear_desc);
        command_list->DrawIndexed(3, 1, 0, 0, 0);
        command_list->EndRenderPass();
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kRenderTarget, ResourceState::kPresent } });
        command_list->Close();
    }

    while (!app.PollEvents()) {
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

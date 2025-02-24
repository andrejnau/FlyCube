#include "AppBox/AppBox.h"
#include "AppSettings/ArgsParser.h"
#include "Instance/Instance.h"

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("DesktopBindless", settings);
    AppSize app_size = app.GetAppSize();

    std::shared_ptr<Instance> instance = CreateInstance(settings.api_type);
    std::shared_ptr<Adapter> adapter = std::move(instance->EnumerateAdapters()[settings.required_gpu_index]);
    app.SetGpuName(adapter->GetName());
    std::shared_ptr<Device> device = adapter->CreateDevice();
    std::shared_ptr<CommandQueue> command_queue = device->GetCommandQueue(CommandListType::kGraphics);
    constexpr uint32_t frame_count = 3;
    std::shared_ptr<Swapchain> swapchain = device->CreateSwapchain(app.GetNativeWindow(), app_size.width(),
                                                                   app_size.height(), frame_count, settings.vsync);
    uint64_t fence_value = 0;
    std::shared_ptr<Fence> fence = device->CreateFence(fence_value);

    std::vector<uint32_t> index_data = { 0, 1, 2 };
    std::shared_ptr<Resource> index_buffer =
        device->CreateBuffer(BindFlag::kCopyDest, sizeof(uint32_t) * index_data.size());
    index_buffer->CommitMemory(MemoryType::kUpload);
    index_buffer->UpdateUploadBuffer(0, index_data.data(), sizeof(index_data.front()) * index_data.size());

    std::vector<glm::vec3> vertex_data = {
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3(0.0, 0.5, 0.0),
        glm::vec3(0.5, -0.5, 0.0),
    };
    std::shared_ptr<Resource> vertex_buffer =
        device->CreateBuffer(BindFlag::kCopyDest, sizeof(vertex_data.front()) * vertex_data.size());
    vertex_buffer->CommitMemory(MemoryType::kUpload);
    vertex_buffer->UpdateUploadBuffer(0, vertex_data.data(), sizeof(vertex_data.front()) * vertex_data.size());

    glm::vec4 pixel_constant_data = glm::vec4(1, 0, 0, 1);
    std::shared_ptr<Resource> pixel_constant_buffer =
        device->CreateBuffer(BindFlag::kConstantBuffer | BindFlag::kCopyDest, sizeof(pixel_constant_data));
    pixel_constant_buffer->CommitMemory(MemoryType::kUpload);
    pixel_constant_buffer->UpdateUploadBuffer(0, &pixel_constant_data, sizeof(pixel_constant_data));

    std::shared_ptr<Shader> vertex_shader =
        device->CompileShader({ ASSETS_PATH "shaders/Bindless/VertexShader.hlsl", "main", ShaderType::kVertex, "6_0" });
    std::shared_ptr<Shader> pixel_shader =
        device->CompileShader({ ASSETS_PATH "shaders/Bindless/PixelShader.hlsl", "main", ShaderType::kPixel, "6_0" });
    std::shared_ptr<Program> program = device->CreateProgram({ vertex_shader, pixel_shader });

    ViewDesc bindless_view_desc = {};
    bindless_view_desc.view_type = ViewType::kStructuredBuffer;
    bindless_view_desc.dimension = ViewDimension::kBuffer;
    bindless_view_desc.bindless = true;
    bindless_view_desc.structure_stride = 4;
    std::shared_ptr<View> index_buffer_view = device->CreateView(index_buffer, bindless_view_desc);
    bindless_view_desc.structure_stride = 12;
    std::shared_ptr<View> vertex_buffer_view = device->CreateView(vertex_buffer, bindless_view_desc);

    std::pair<uint32_t, uint32_t> vertex_constant_data = { index_buffer_view->GetDescriptorId(),
                                                           vertex_buffer_view->GetDescriptorId() };
    std::shared_ptr<Resource> vertex_constant_buffer =
        device->CreateBuffer(BindFlag::kConstantBuffer | BindFlag::kCopyDest, sizeof(pixel_constant_data));
    vertex_constant_buffer->CommitMemory(MemoryType::kUpload);
    vertex_constant_buffer->UpdateUploadBuffer(0, &vertex_constant_data, sizeof(vertex_constant_data));

    ViewDesc constant_buffer_view_desc = {};
    constant_buffer_view_desc.view_type = ViewType::kConstantBuffer;
    constant_buffer_view_desc.dimension = ViewDimension::kBuffer;
    std::shared_ptr<View> pixel_constant_buffer_view =
        device->CreateView(pixel_constant_buffer, constant_buffer_view_desc);
    std::shared_ptr<View> vertex_constant_buffer_view =
        device->CreateView(vertex_constant_buffer, constant_buffer_view_desc);

    BindKey pixel_constant_buffer_key = { ShaderType::kPixel, ViewType::kConstantBuffer,
                                          /*slot=*/1,
                                          /*space=*/0,
                                          /*count=*/1 };
    BindKey vertex_constant_buffer_key = { ShaderType::kVertex, ViewType::kConstantBuffer,
                                           /*slot=*/0,
                                           /*space=*/0,
                                           /*count=*/1 };
    BindKey vertex_structured_buffers_uint_key = { ShaderType::kVertex, ViewType::kStructuredBuffer,
                                                   /*slot=*/0,
                                                   /*space=*/1,
                                                   /*count=*/kBindlessCount };
    BindKey vertex_structured_buffers_float3_key = { ShaderType::kVertex, ViewType::kStructuredBuffer,
                                                     /*slot=*/0,
                                                     /*space=*/2,
                                                     /*count=*/kBindlessCount };
    std::shared_ptr<BindingSetLayout> layout =
        device->CreateBindingSetLayout({ pixel_constant_buffer_key, vertex_constant_buffer_key,
                                         vertex_structured_buffers_uint_key, vertex_structured_buffers_float3_key });
    std::shared_ptr<BindingSet> binding_set = device->CreateBindingSet(layout);
    binding_set->WriteBindings({ { pixel_constant_buffer_key, pixel_constant_buffer_view },
                                 { vertex_constant_buffer_key, vertex_constant_buffer_view } });

    RenderPassDesc render_pass_desc = {
        { { swapchain->GetFormat(), RenderPassLoadOp::kClear, RenderPassStoreOp::kStore } },
    };
    std::shared_ptr<RenderPass> render_pass = device->CreateRenderPass(render_pass_desc);
    ClearDesc clear_desc = { { { 0.0, 0.2, 0.4, 1.0 } } };
    GraphicsPipelineDesc pipeline_desc = {
        program,
        layout,
        {},
        render_pass,
    };
    std::shared_ptr<Pipeline> pipeline = device->CreateGraphicsPipeline(pipeline_desc);

    std::array<uint64_t, frame_count> fence_values = {};
    std::vector<std::shared_ptr<CommandList>> command_lists;
    std::vector<std::shared_ptr<Framebuffer>> framebuffers;
    for (uint32_t i = 0; i < frame_count; ++i) {
        ViewDesc back_buffer_view_desc = {};
        back_buffer_view_desc.view_type = ViewType::kRenderTarget;
        back_buffer_view_desc.dimension = ViewDimension::kTexture2D;
        std::shared_ptr<Resource> back_buffer = swapchain->GetBackBuffer(i);
        std::shared_ptr<View> back_buffer_view = device->CreateView(back_buffer, back_buffer_view_desc);
        FramebufferDesc framebuffer_desc = {};
        framebuffer_desc.render_pass = render_pass;
        framebuffer_desc.width = app_size.width();
        framebuffer_desc.height = app_size.height();
        framebuffer_desc.colors = { back_buffer_view };
        std::shared_ptr<Framebuffer> framebuffer =
            framebuffers.emplace_back(device->CreateFramebuffer(framebuffer_desc));
        std::shared_ptr<CommandList> command_list =
            command_lists.emplace_back(device->CreateCommandList(CommandListType::kGraphics));
        command_list->BindPipeline(pipeline);
        command_list->BindBindingSet(binding_set);
        command_list->SetViewport(0, 0, app_size.width(), app_size.height());
        command_list->SetScissorRect(0, 0, app_size.width(), app_size.height());
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kRenderTarget } });
        command_list->BeginRenderPass(render_pass, framebuffer, clear_desc);
        command_list->Draw(3, 1, 0, 0);
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

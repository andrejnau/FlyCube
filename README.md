# FlyCube
FlyCube is a low-level graphics API is written in C++ on top of `DirectX 12`, `Vulkan` and `Metal 4`.

### The low-level graphics API features
* Ray tracing
* Mesh shading
* Variable rate shading
* Bindless resource binding
* HLSL as a shader language for all backends
  * Compilation in DXIL, SPIRV or MSL depend on selected backend

### Supported platforms

|                | DirectX 12               | Vulkan                        | Metal 4                  |
|----------------|--------------------------|-------------------------------|--------------------------|
| Windows        | :heavy_check_mark:       | :heavy_check_mark:            | :heavy_multiplication_x: |
| macOS/iOS/tvOS | :heavy_multiplication_x: | :heavy_check_mark: (MoltenVK) | :heavy_check_mark:       |
| Linux/Android  | :heavy_multiplication_x: | :heavy_check_mark:            | :heavy_multiplication_x: |

### Cloning repository
```
git clone --recursive https://github.com/andrejnau/FlyCube.git
```

### An example of the low-level graphics API usage
```cpp
std::shared_ptr<Instance> instance = CreateInstance(settings.api_type);
std::shared_ptr<Adapter> adapter = std::move(instance->EnumerateAdapters()[settings.required_gpu_index]);
std::shared_ptr<Device> device = adapter->CreateDevice();
std::shared_ptr<CommandQueue> command_queue = device->GetCommandQueue(CommandListType::kGraphics);
std::shared_ptr<Swapchain> swapchain =
    device->CreateSwapchain(app.GetNativeWindow(), app_size.width(), app_size.height(), kFrameCount, settings.vsync);
uint64_t fence_value = 0;
std::shared_ptr<Fence> fence = device->CreateFence(fence_value);

std::vector<uint32_t> index_data = { 0, 1, 2 };
std::shared_ptr<Resource> index_buffer =
    device->CreateBuffer(MemoryType::kUpload,
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
std::shared_ptr<Resource> constant_buffer =
    device->CreateBuffer(MemoryType::kUpload, { .size = sizeof(constant_data), .usage = BindFlag::kConstantBuffer });
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

GraphicsPipelineDesc pipeline_desc = {
    .program = device->CreateProgram({ vertex_shader, pixel_shader }),
    .layout = layout,
    .input = { { 0, "POSITION", gli::FORMAT_RGB32_SFLOAT_PACK32, sizeof(vertex_data.front()) } },
    .color_formats = { swapchain->GetFormat() },
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

    auto& command_list = command_lists[i];
    command_list = device->CreateCommandList(CommandListType::kGraphics);
    command_list->BindPipeline(pipeline);
    command_list->BindBindingSet(binding_set);
    command_list->SetViewport(0, 0, app_size.width(), app_size.height());
    command_list->SetScissorRect(0, 0, app_size.width(), app_size.height());
    command_list->IASetIndexBuffer(index_buffer, 0, gli::format::FORMAT_R32_UINT_PACK32);
    command_list->IASetVertexBuffer(0, vertex_buffer, 0);
    command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kRenderTarget } });
    RenderPassDesc render_pass_desc = {
        .colors = { { .load_op = RenderPassLoadOp::kClear,
                      .store_op = RenderPassStoreOp::kStore,
                      .clear_value = glm::vec4(0.0, 0.2, 0.4, 1.0) } },
    };
    FramebufferDesc framebuffer_desc = {
        .width = app_size.width(),
        .height = app_size.height(),
        .colors = { back_buffer_views[i] },
    };
    command_list->BeginRenderPass(render_pass_desc, framebuffer_desc);
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
```

### Advanced sample
[SponzaPbr](https://github.com/andrejnau/SponzaPbr) was originally part of the repository. This is my sandbox for rendering techniques.
* Features
  * Deferred rendering
  * Physically based rendering
  * Image based lighting
  * Ambient occlusion
    * Raytracing
    * Screen space
  * Normal mapping
  * Point shadow mapping
  * Skeletal animation
  * Multisample anti-aliasing
  * Tone mapping
  * Simple imgui based UI settings

![sponza.png](screenshots/sponza.png)

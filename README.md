# FlyCube
FlyCube is a low-level graphics API is written in C++ on top of `DirectX 12`, `Vulkan` and `Metal`.

### The low-level graphics API features
* Ray tracing
* Mesh shading
* Variable rate shading
* Bindless resource binding
* HLSL as a shader language for all backends
  * Compilation in DXIL, SPIRV or MSL depend on selected backend

### Cloning repository
```
git clone --recursive https://github.com/andrejnau/FlyCube.git
```

### Build requirements
* Windows SDK Version 10.0.19041.0

### An example of the low-level graphics API usage
```cpp
std::shared_ptr<Instance> instance = CreateInstance(settings.api_type);
std::shared_ptr<Adapter> adapter = std::move(instance->EnumerateAdapters()[settings.required_gpu_index]);
std::shared_ptr<Device> device = adapter->CreateDevice();
std::shared_ptr<CommandQueue> command_queue = device->GetCommandQueue(CommandListType::kGraphics);
constexpr uint32_t frame_count = 3;
std::shared_ptr<Swapchain> swapchain = device->CreateSwapchain(app.GetNativeWindow(), rect.width, rect.height, frame_count, settings.vsync);
uint64_t fence_value = 0;
std::shared_ptr<Fence> fence = device->CreateFence(fence_value);

std::vector<uint32_t> index_data = { 0, 1, 2 };
std::shared_ptr<Resource> index_buffer = device->CreateBuffer(BindFlag::kIndexBuffer | BindFlag::kCopyDest, sizeof(uint32_t) * index_data.size());
index_buffer->CommitMemory(MemoryType::kUpload);
index_buffer->UpdateUploadBuffer(0, index_data.data(), sizeof(index_data.front()) * index_data.size());

std::vector<glm::vec3> vertex_data = { glm::vec3(-0.5, -0.5, 0.0), glm::vec3(0.0,  0.5, 0.0), glm::vec3(0.5, -0.5, 0.0) };
std::shared_ptr<Resource> vertex_buffer = device->CreateBuffer(BindFlag::kVertexBuffer | BindFlag::kCopyDest, sizeof(vertex_data.front()) * vertex_data.size());
vertex_buffer->CommitMemory(MemoryType::kUpload);
vertex_buffer->UpdateUploadBuffer(0, vertex_data.data(), sizeof(vertex_data.front()) * vertex_data.size());

glm::vec4 constant_data = glm::vec4(1, 0, 0, 1);
std::shared_ptr<Resource> constant_buffer = device->CreateBuffer(BindFlag::kConstantBuffer | BindFlag::kCopyDest, sizeof(constant_data));
constant_buffer->CommitMemory(MemoryType::kUpload);
constant_buffer->UpdateUploadBuffer(0, &constant_data, sizeof(constant_data));

std::shared_ptr<Shader> vertex_shader = device->CompileShader({ ASSETS_PATH"shaders/CoreTriangle/VertexShader.hlsl", "main", ShaderType::kVertex, "6_0" });
std::shared_ptr<Shader> pixel_shader = device->CompileShader({ ASSETS_PATH"shaders/CoreTriangle/PixelShader.hlsl", "main",  ShaderType::kPixel, "6_0" });
std::shared_ptr<Program> program = device->CreateProgram({ vertex_shader, pixel_shader });

ViewDesc constant_view_desc = {};
constant_view_desc.view_type = ViewType::kConstantBuffer;
constant_view_desc.dimension = ViewDimension::kBuffer;
std::shared_ptr<View> constant_view = device->CreateView(constant_buffer, constant_view_desc);
BindKey settings_key = { ShaderType::kPixel, ViewType::kConstantBuffer, 0, 0, 1 };
std::shared_ptr<BindingSetLayout> layout = device->CreateBindingSetLayout({ settings_key });
std::shared_ptr<BindingSet> binding_set = device->CreateBindingSet(layout);
binding_set->WriteBindings({ { settings_key, constant_view } });

RenderPassDesc render_pass_desc = {
    { { swapchain->GetFormat(), RenderPassLoadOp::kClear, RenderPassStoreOp::kStore } },
};
std::shared_ptr<RenderPass> render_pass = device->CreateRenderPass(render_pass_desc);
ClearDesc clear_desc = { { { 0.0, 0.2, 0.4, 1.0 } } };
GraphicsPipelineDesc pipeline_desc = {
    program,
    layout,
    { { 0, "POSITION", gli::FORMAT_RGB32_SFLOAT_PACK32, sizeof(vertex_data.front()) } },
    render_pass
};
std::shared_ptr<Pipeline> pipeline = device->CreateGraphicsPipeline(pipeline_desc);

std::array<uint64_t, frame_count> fence_values = {};
std::vector<std::shared_ptr<CommandList>> command_lists;
std::vector<std::shared_ptr<Framebuffer>> framebuffers;
for (uint32_t i = 0; i < frame_count; ++i)
{
    ViewDesc back_buffer_view_desc = {};
    back_buffer_view_desc.view_type = ViewType::kRenderTarget;
    back_buffer_view_desc.dimension = ViewDimension::kTexture2D;
    std::shared_ptr<Resource> back_buffer = swapchain->GetBackBuffer(i);
    std::shared_ptr<View> back_buffer_view = device->CreateView(back_buffer, back_buffer_view_desc);
    FramebufferDesc framebuffer_desc = {};
    framebuffer_desc.render_pass = render_pass;
    framebuffer_desc.width = rect.width;
    framebuffer_desc.height = rect.height;
    framebuffer_desc.colors = { back_buffer_view };
    std::shared_ptr<Framebuffer> framebuffer = framebuffers.emplace_back(device->CreateFramebuffer(framebuffer_desc));
    std::shared_ptr<CommandList> command_list = command_lists.emplace_back(device->CreateCommandList(CommandListType::kGraphics));
    command_list->BindPipeline(pipeline);
    command_list->BindBindingSet(binding_set);
    command_list->SetViewport(0, 0, rect.width, rect.height);
    command_list->SetScissorRect(0, 0, rect.width, rect.height);
    command_list->IASetIndexBuffer(index_buffer, gli::format::FORMAT_R32_UINT_PACK32);
    command_list->IASetVertexBuffer(0, vertex_buffer);
    command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kRenderTarget } });
    command_list->BeginRenderPass(render_pass, framebuffer, clear_desc);
    command_list->DrawIndexed(3, 1, 0, 0, 0);
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

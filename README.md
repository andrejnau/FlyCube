## Low-level graphics api
This api is written as C++ wrapper over existing graphics api. The api provide most of their features but hide some details.

* Backends
  * DirectX 12
  * Vulkan

* Supported features
  * Traditional graphics/compute pipeline
  * Ray Tracing
  * HLSL as shading language for all backends
    * Compilation in DXBC, DXIL, SPIRV

* Planned features
  * Mesh shading (need investigation)
  * DX11/OpenGL were available on [commit](https://github.com/andrejnau/FlyCube/tree/9756f8fae2530a635302c549694374206c886b5c), not sure if these api really needs

* Platforms
  * Windows 10
  
### Low-level graphics api example
```cpp
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
```

## High-level graphics api and utilities
A set of classes simplifying the writing of complex scenes.

* High-level api features
  * Generated shader helper by shader reflection
    * Easy to use resources binding
    * Constant buffers proxy for compile time access to members

* Utilities features
  * Application skeleton
    * Window creating
    * Keyboard/Mouse events source
  * Camera
  * Geometry utils
    * 3D models loading with assimp
  * Texture utils
    * Images loading with gli and SOIL
   
### High-level graphics api example
```cpp
Settings settings = ParseArgs(argc, argv);
AppBox app("Triangle", settings);

Context context(settings, app.GetWindow());
AppRect rect = app.GetAppRect();
ProgramHolder<PixelShaderPS, VertexShaderVS> program(context);

std::vector<uint32_t> ibuf = { 0, 1, 2 };
std::shared_ptr<Resource> index = context.CreateBuffer(BindFlag::kIbv, sizeof(uint32_t) * ibuf.size(), sizeof(uint32_t));
context.UpdateSubresource(index, 0, ibuf.data(), 0, 0);
std::vector<glm::vec3> pbuf = {
    glm::vec3(-0.5, -0.5, 0.0),
    glm::vec3(0.0,  0.5, 0.0),
    glm::vec3(0.5, -0.5, 0.0)
};
std::shared_ptr<Resource> pos = context.CreateBuffer(BindFlag::kVbv, sizeof(glm::vec3) * pbuf.size(), sizeof(glm::vec3));
context.UpdateSubresource(pos, 0, pbuf.data(), 0, 0);

while (!app.PollEvents())
{
    context.UseProgram(program);
    context.SetViewport(rect.width, rect.height);
    program.ps.om.rtv0.Attach(context.GetBackBuffer()).Clear({ 0.0f, 0.2f, 0.4f, 1.0f });
    context.IASetIndexBuffer(index, gli::format::FORMAT_R32_UINT_PACK32);
    context.IASetVertexBuffer(program.vs.ia.POSITION, pos);
    program.ps.cbuffer.Settings.color = glm::vec4(1, 0, 0, 1);
    context.DrawIndexed(3, 0, 0);
    context.Present();
    app.UpdateFps();
}
```

## SponzaPbr

* Scene Features
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

### SponzaPbr Settings
Press Tab to open settings menu

## Build
```
python init.py
mkdir build
cd build
cmake -G "Visual Studio 16 2019 Win64" ..
cmake --build . --config RelWithDebInfo
```

## Setup for Vulkan
Download latest DirectX Shader Compiler with support spirv backend from https://ci.appveyor.com/project/antiagainst/directxshadercompiler/branch/master/artifacts
Unzip to 3rdparty/dxc-artifacts

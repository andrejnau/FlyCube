#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <Instance/Instance.h>
#include <stdexcept>

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("CoreDxrTriangle", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<Instance> instance = CreateInstance(settings.api_type);
    std::shared_ptr<Adapter> adapter = std::move(instance->EnumerateAdapters()[settings.required_gpu_index]);
    app.SetGpuName(adapter->GetName());
    std::shared_ptr<Device> device = adapter->CreateDevice();
    if (!device->IsDxrSupported())
        throw std::runtime_error("Ray Tracing is not supported");
    std::shared_ptr<CommandQueue> command_queue = device->GetCommandQueue(CommandListType::kGraphics);
    std::shared_ptr<CommandQueue> upload_command_queue = device->GetCommandQueue(CommandListType::kGraphics);
    constexpr uint32_t frame_count = 3;
    std::shared_ptr<Swapchain> swapchain = device->CreateSwapchain(app.GetWindow(), rect.width, rect.height, frame_count, settings.vsync);
    uint64_t fence_value = 0;
    std::shared_ptr<Fence> fence = device->CreateFence(fence_value);

    std::vector<uint32_t> index_data = { 0, 1, 2 };
    std::shared_ptr<Resource> index_buffer = device->CreateBuffer(BindFlag::kIndexBuffer | BindFlag::kCopyDest, sizeof(uint32_t) * index_data.size());
    index_buffer->CommitMemory(MemoryType::kDefault);
    index_buffer->SetName("index_buffer");
    std::vector<glm::vec3> vertex_data = { glm::vec3(-0.5, -0.5, 0.0), glm::vec3(0.0,  0.5, 0.0), glm::vec3(0.5, -0.5, 0.0) };
    std::shared_ptr<Resource> vertex_buffer = device->CreateBuffer(BindFlag::kVertexBuffer | BindFlag::kCopyDest, sizeof(vertex_data.front()) * vertex_data.size());
    vertex_buffer->CommitMemory(MemoryType::kDefault);
    vertex_buffer->SetName("vertex_buffer");

    std::shared_ptr<Resource> upload_buffer = device->CreateBuffer(BindFlag::kCopySource, index_buffer->GetWidth() + vertex_buffer->GetWidth());
    upload_buffer->CommitMemory(MemoryType::kUpload);
    upload_buffer->SetName("upload_buffer");
    upload_buffer->UpdateUploadBuffer(0, index_data.data(), sizeof(index_data.front()) * index_data.size());
    upload_buffer->UpdateUploadBuffer(index_buffer->GetWidth(), vertex_data.data(), sizeof(vertex_data.front()) * vertex_data.size());

    std::shared_ptr<CommandList> upload_command_list = device->CreateCommandList(CommandListType::kGraphics);
    upload_command_list->CopyBuffer(upload_buffer, index_buffer, { { 0, 0, index_buffer->GetWidth() } });
    upload_command_list->CopyBuffer(upload_buffer, vertex_buffer, { { index_buffer->GetWidth(), 0, vertex_buffer->GetWidth() } });

    upload_command_list->ResourceBarrier({ { index_buffer, ResourceState::kCopyDest, ResourceState::kNonPixelShaderResource } });
    upload_command_list->ResourceBarrier({ { vertex_buffer, ResourceState::kCopyDest, ResourceState::kNonPixelShaderResource } });
    RaytracingGeometryDesc raytracing_geometry_desc = {
        { vertex_buffer, gli::format::FORMAT_RGB32_SFLOAT_PACK32, 3 },
        { index_buffer, gli::format::FORMAT_R32_UINT_PACK32, 3 },
        RaytracingGeometryFlags::kOpaque
    };
    std::shared_ptr<Resource> bottom = device->CreateBottomLevelAS({ raytracing_geometry_desc }, BuildAccelerationStructureFlags::kAllowCompaction);
    bottom->CommitMemory(MemoryType::kDefault);
    bottom->SetName("bottom");

    std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4>> geometry = {
        { bottom, glm::mat4(1.0) },
    };
    std::vector<RaytracingGeometryInstance> instances;
    for (const auto& mesh : geometry)
    {
        RaytracingGeometryInstance& instance = instances.emplace_back();
        memcpy(&instance.transform, &mesh.second, sizeof(instance.transform));
        instance.instance_id = static_cast<uint32_t>(instances.size() - 1);
        instance.instance_mask = 0xff;
        instance.acceleration_structure_handle = mesh.first->GetAccelerationStructureHandle();
    }

    std::shared_ptr<Resource> top = device->CreateTopLevelAS(instances.size(), BuildAccelerationStructureFlags::kNone);
    top->CommitMemory(MemoryType::kDefault);
    top->SetName("top");
    RaytracingASPrebuildInfo prebuild_info_top = top->GetRaytracingASPrebuildInfo();
    RaytracingASPrebuildInfo prebuild_info_bottom = bottom->GetRaytracingASPrebuildInfo();

    auto scratch = device->CreateBuffer(BindFlag::kRayTracing, prebuild_info_bottom.build_scratch_data_size + prebuild_info_top.build_scratch_data_size);
    scratch->CommitMemory(MemoryType::kDefault);
    scratch->SetName("scratch");

    upload_command_list->BuildBottomLevelAS({}, bottom, scratch, 0, { raytracing_geometry_desc });
    upload_command_list->UAVResourceBarrier(bottom);
    upload_command_list->CopyAccelerationStructure(bottom, bottom, CopyAccelerationStructureMode::kCompact);

    auto instance_data = device->CreateBuffer(BindFlag::kRayTracing, instances.size() * sizeof(instances.back()));
    instance_data->CommitMemory(MemoryType::kUpload);
    instance_data->SetName("instance_data");
    instance_data->UpdateUploadBuffer(0, instances.data(), instances.size() * sizeof(instances.back()));
    upload_command_list->BuildTopLevelAS({}, top, scratch, prebuild_info_bottom.build_scratch_data_size, instance_data, 0, instances.size());
    upload_command_list->UAVResourceBarrier(top);

    std::shared_ptr<Resource> uav = device->CreateTexture(BindFlag::kUnorderedAccess | BindFlag::kCopySource, swapchain->GetFormat(), 1, rect.width, rect.height);
    uav->CommitMemory(MemoryType::kDefault);
    uav->SetName("uav");
    upload_command_list->ResourceBarrier({ { uav, uav->GetInitialState(), ResourceState::kUnorderedAccess } });
    upload_command_list->Close();

    upload_command_queue->ExecuteCommandLists({ upload_command_list });
    upload_command_queue->Signal(fence, ++fence_value);
    command_queue->Wait(fence, fence_value);

    ViewDesc top_view_desc = {};
    top_view_desc.view_type = ViewType::kAccelerationStructure;
    top_view_desc.dimension = ResourceDimension::kRaytracingAccelerationStructure;
    std::shared_ptr<View> top_view = device->CreateView(top, top_view_desc);

    ViewDesc uav_view_desc = {};
    uav_view_desc.view_type = ViewType::kTexture;
    uav_view_desc.dimension = ResourceDimension::kTexture2D;
    std::shared_ptr<View> uav_view = device->CreateView(uav, uav_view_desc);

    std::shared_ptr<Shader> library = device->CompileShader({ ASSETS_PATH"shaders/DxrTriangle/RayTracing.hlsl", "", ShaderType::kLibrary, "6_3" });
    std::shared_ptr<Program> program = device->CreateProgram({ library });
    BindKey geometry_key = library->GetBindKey("geometry");
    BindKey result_key = library->GetBindKey("result");
    std::shared_ptr<BindingSetLayout> layout = device->CreateBindingSetLayout({ geometry_key, result_key });
    std::shared_ptr<BindingSet> binding_set = device->CreateBindingSet(layout);
    binding_set->WriteBindings({
        { geometry_key, top_view },
        { result_key, uav_view }
    });

    std::shared_ptr<Pipeline> pipeline = device->CreateRayTracingPipeline({ program, layout });
    std::array<uint64_t, frame_count> fence_values = {};
    std::vector<std::shared_ptr<CommandList>> command_lists;
    for (uint32_t i = 0; i < frame_count; ++i)
    {
        std::shared_ptr<Resource> back_buffer = swapchain->GetBackBuffer(i);
        ViewDesc back_buffer_view_desc = {};
        back_buffer_view_desc.view_type = ViewType::kRenderTarget;
        std::shared_ptr<View> back_buffer_view = device->CreateView(back_buffer, back_buffer_view_desc);
        command_lists.emplace_back(device->CreateCommandList(CommandListType::kGraphics));
        std::shared_ptr<CommandList> command_list = command_lists[i];
        command_list->BindPipeline(pipeline);
        command_list->BindBindingSet(binding_set);
        command_list->DispatchRays(rect.width, rect.height, 1);
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kCopyDest } });
        command_list->ResourceBarrier({ { uav, ResourceState::kUnorderedAccess, ResourceState::kCopySource } });
        command_list->CopyTexture(uav, back_buffer, { { rect.width, rect.height, 1 } });
        command_list->ResourceBarrier({ { uav, ResourceState::kCopySource, ResourceState::kUnorderedAccess } });
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kCopyDest, ResourceState::kPresent } });
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

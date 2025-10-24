#include "AppBox/AppBox.h"
#include "AppSettings/ArgsParser.h"
#include "Instance/Instance.h"

#include <chrono>

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("DesktopDispatchIndirect", settings);
    AppSize app_size = app.GetAppSize();

    std::shared_ptr<Instance> instance = CreateInstance(settings.api_type);
    std::shared_ptr<Adapter> adapter = std::move(instance->EnumerateAdapters()[settings.required_gpu_index]);
    app.SetGpuName(adapter->GetName());
    std::shared_ptr<Device> device = adapter->CreateDevice();
    std::shared_ptr<CommandQueue> command_queue = device->GetCommandQueue(CommandListType::kGraphics);
    static constexpr uint32_t kFrameCount = 3;
    std::shared_ptr<Swapchain> swapchain = device->CreateSwapchain(app.GetNativeWindow(), app_size.width(),
                                                                   app_size.height(), kFrameCount, settings.vsync);
    uint64_t fence_value = 0;
    std::shared_ptr<Fence> fence = device->CreateFence(fence_value);

    std::array<float, kFrameCount> cbv_data = {};
    std::array<std::shared_ptr<Resource>, kFrameCount> cbv_buffer = {};
    std::array<std::shared_ptr<View>, kFrameCount> cbv_view = {};
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        cbv_buffer[i] = device->CreateBuffer(MemoryType::kUpload, {
                                                                      .size = sizeof(float),
                                                                      .usage = BindFlag::kConstantBuffer,
                                                                  });
        cbv_buffer[i]->CommitMemory(MemoryType::kUpload);
        ViewDesc cbv_view_desc = {
            .view_type = ViewType::kConstantBuffer,
            .dimension = ViewDimension::kBuffer,
        };
        cbv_view[i] = device->CreateView(cbv_buffer[i], cbv_view_desc);
    }

    static constexpr uint32_t kUavWidth = 512;
    static constexpr uint32_t kUavHeight = 512;
    std::shared_ptr<Resource> uav_texture =
        device->CreateTexture(MemoryType::kDefault, {
                                                        .type = TextureType::k2D,
                                                        .format = swapchain->GetFormat(),
                                                        .width = kUavWidth,
                                                        .height = kUavHeight,
                                                        .depth_or_array_layers = 1,
                                                        .mip_levels = 1,
                                                        .sample_count = 1,
                                                        .usage = BindFlag::kUnorderedAccess | BindFlag::kCopySource,
                                                    });
    uav_texture->CommitMemory(MemoryType::kDefault);
    ViewDesc uav_view_desc = {
        .view_type = ViewType::kRWTexture,
        .dimension = ViewDimension::kTexture2D,
    };
    std::shared_ptr<View> uav_view = device->CreateView(uav_texture, uav_view_desc);

    DispatchIndirectCommand argument_data = { 64, 64, 1 };
    std::shared_ptr<Resource> argument_buffer =
        device->CreateBuffer(MemoryType::kUpload, {
                                                      .size = sizeof(argument_data),
                                                      .usage = BindFlag::kIndirectBuffer,
                                                  });
    argument_buffer->CommitMemory(MemoryType::kUpload);
    argument_buffer->UpdateUploadBuffer(0, &argument_data, sizeof(argument_data));

    std::shared_ptr<Shader> shader = device->CompileShader(
        { ASSETS_PATH "shaders/DispatchIndirect/ComputeShader.hlsl", "main", ShaderType::kCompute, "6_0" });
    BindKey cbv_key = shader->GetBindKey("cbv");
    BindKey uav_key = shader->GetBindKey("uav");
    std::shared_ptr<BindingSetLayout> layout = device->CreateBindingSetLayout({ cbv_key, uav_key });
    ComputePipelineDesc pipeline_desc = {
        device->CreateProgram({ shader }),
        layout,
    };
    std::shared_ptr<Pipeline> pipeline = device->CreateComputePipeline(pipeline_desc);

    std::array<uint64_t, kFrameCount> fence_values = {};
    std::array<std::shared_ptr<BindingSet>, kFrameCount> binding_set = {};
    std::array<std::shared_ptr<CommandList>, kFrameCount> command_lists = {};
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        std::shared_ptr<Resource> back_buffer = swapchain->GetBackBuffer(i);
        binding_set[i] = device->CreateBindingSet(layout);
        binding_set[i]->WriteBindings({ { cbv_key, cbv_view[i] }, { uav_key, uav_view } });
        command_lists[i] = device->CreateCommandList(CommandListType::kGraphics);
        auto& command_list = command_lists[i];

        command_list->BindPipeline(pipeline);
        command_list->BindBindingSet(binding_set[i]);
        command_list->DispatchIndirect(argument_buffer, 0);
        TextureCopyRegion region = {
            .extent = { kUavWidth, kUavHeight, 1 },
            .dst_offset = { static_cast<int32_t>(app_size.width() / 2 - kUavWidth / 2),
                            static_cast<int32_t>(app_size.height() / 2 - kUavHeight / 2) },
        };
        command_list->ResourceBarrier({ { uav_texture, ResourceState::kUnorderedAccess, ResourceState::kCopySource },
                                        { back_buffer, ResourceState::kPresent, ResourceState::kCopyDest } });
        command_list->CopyTexture(uav_texture, back_buffer, { region });
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kCopyDest, ResourceState::kPresent },
                                        { uav_texture, ResourceState::kCopySource, ResourceState::kUnorderedAccess } });
        command_list->Close();
    }

    while (!app.PollEvents()) {
        uint32_t frame_index = swapchain->NextImage(fence, ++fence_value);
        command_queue->Wait(fence, fence_value);
        fence->Wait(fence_values[frame_index]);

        auto now = std::chrono::high_resolution_clock::now();
        cbv_data[frame_index] = std::chrono::duration<float>(now.time_since_epoch()).count();
        cbv_buffer[frame_index]->UpdateUploadBuffer(0, &cbv_data[frame_index], sizeof(cbv_data.front()));

        command_queue->ExecuteCommandLists({ command_lists[frame_index] });
        command_queue->Signal(fence, fence_values[frame_index] = ++fence_value);
        swapchain->Present(fence, fence_values[frame_index]);
    }
    command_queue->Signal(fence, ++fence_value);
    fence->Wait(fence_value);
    return 0;
}

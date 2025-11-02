#include "AppBox/AppBox.h"
#include "AppSettings/ArgsParser.h"
#include "Instance/Instance.h"

#include <chrono>

namespace {

constexpr uint32_t kFrameCount = 3;

} // namespace

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
    std::shared_ptr<Swapchain> swapchain = device->CreateSwapchain(app.GetNativeWindow(), app_size.width(),
                                                                   app_size.height(), kFrameCount, settings.vsync);
    uint64_t fence_value = 0;
    std::shared_ptr<Fence> fence = device->CreateFence(fence_value);

    std::array<float, kFrameCount> constant_buffer_data = {};
    std::array<std::shared_ptr<Resource>, kFrameCount> constant_buffer = {};
    std::array<std::shared_ptr<View>, kFrameCount> constant_buffer_view = {};
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        constant_buffer[i] =
            device->CreateBuffer(MemoryType::kUpload, { .size = sizeof(float), .usage = BindFlag::kConstantBuffer });
        ViewDesc constant_buffer_view_desc = {
            .view_type = ViewType::kConstantBuffer,
            .dimension = ViewDimension::kBuffer,
        };
        constant_buffer_view[i] = device->CreateView(constant_buffer[i], constant_buffer_view_desc);
    }

    TextureDesc result_texture_desc = {
        .type = TextureType::k2D,
        .format = swapchain->GetFormat(),
        .width = 512,
        .height = 512,
        .depth_or_array_layers = 1,
        .mip_levels = 1,
        .sample_count = 1,
        .usage = BindFlag::kUnorderedAccess | BindFlag::kCopySource,
    };
    std::shared_ptr<Resource> result_texture = device->CreateTexture(MemoryType::kDefault, result_texture_desc);
    ViewDesc result_texture_view_desc = {
        .view_type = ViewType::kRWTexture,
        .dimension = ViewDimension::kTexture2D,
    };
    std::shared_ptr<View> result_texture_view = device->CreateView(result_texture, result_texture_view_desc);

    DispatchIndirectCommand argument_data = { 64, 64, 1 };
    std::shared_ptr<Resource> argument_buffer = device->CreateBuffer(
        MemoryType::kUpload, { .size = sizeof(argument_data), .usage = BindFlag::kIndirectBuffer });
    argument_buffer->UpdateUploadBuffer(0, &argument_data, sizeof(argument_data));

    std::shared_ptr<Shader> shader = device->CompileShader(
        { ASSETS_PATH "shaders/DispatchIndirect/ComputeShader.hlsl", "main", ShaderType::kCompute, "6_0" });
    BindKey constant_buffer_key = shader->GetBindKey("constant_buffer");
    BindKey result_texture_key = shader->GetBindKey("result_texture");
    std::shared_ptr<BindingSetLayout> layout =
        device->CreateBindingSetLayout({ constant_buffer_key, result_texture_key });
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
        binding_set[i]->WriteBindings(
            { { constant_buffer_key, constant_buffer_view[i] }, { result_texture_key, result_texture_view } });

        auto& command_list = command_lists[i];
        command_list = device->CreateCommandList(CommandListType::kGraphics);
        command_list->BindPipeline(pipeline);
        command_list->BindBindingSet(binding_set[i]);
        command_list->DispatchIndirect(argument_buffer, 0);
        TextureCopyRegion region = {
            .extent = { result_texture_desc.width, result_texture_desc.height, 1 },
            .dst_offset = { static_cast<int32_t>(app_size.width() / 2 - result_texture_desc.width / 2),
                            static_cast<int32_t>(app_size.height() / 2 - result_texture_desc.height / 2) },
        };
        command_list->ResourceBarrier({ { result_texture, ResourceState::kUnorderedAccess, ResourceState::kCopySource },
                                        { back_buffer, ResourceState::kPresent, ResourceState::kCopyDest } });
        command_list->CopyTexture(result_texture, back_buffer, { region });
        command_list->ResourceBarrier(
            { { back_buffer, ResourceState::kCopyDest, ResourceState::kPresent },
              { result_texture, ResourceState::kCopySource, ResourceState::kUnorderedAccess } });
        command_list->Close();
    }

    while (!app.PollEvents()) {
        uint32_t frame_index = swapchain->NextImage(fence, ++fence_value);
        command_queue->Wait(fence, fence_value);
        fence->Wait(fence_values[frame_index]);

        auto now = std::chrono::high_resolution_clock::now();
        constant_buffer_data[frame_index] = std::chrono::duration<float>(now.time_since_epoch()).count();
        constant_buffer[frame_index]->UpdateUploadBuffer(0, &constant_buffer_data[frame_index],
                                                         sizeof(constant_buffer_data.front()));

        command_queue->ExecuteCommandLists({ command_lists[frame_index] });
        command_queue->Signal(fence, fence_values[frame_index] = ++fence_value);
        swapchain->Present(fence, fence_values[frame_index]);
    }
    command_queue->Signal(fence, ++fence_value);
    fence->Wait(fence_value);
    return 0;
}

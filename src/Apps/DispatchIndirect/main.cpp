#include "ApiType/ApiType.h"
#include "AppLoop/AppLoop.h"
#include "AppSettings/ArgsParser.h"
#include "Instance/Instance.h"
#include "Utilities/Asset.h"

namespace {

constexpr uint32_t kFrameCount = 3;

} // namespace

class DispatchIndirectRenderer : public AppRenderer {
public:
    DispatchIndirectRenderer(const Settings& settings);
    ~DispatchIndirectRenderer() override;

    void Init(const AppSize& app_size, WindowHandle window) override;
    void Resize(const AppSize& app_size, WindowHandle window) override;
    void Render() override;
    std::string_view GetTitle() const override;
    const std::string& GetGpuName() const override;
    const Settings& GetSettings() const override;

private:
    void WaitForIdle();

    Settings m_settings;
    std::shared_ptr<Instance> m_instance;
    std::shared_ptr<Adapter> m_adapter;
    std::shared_ptr<Device> m_device;
    std::shared_ptr<CommandQueue> m_command_queue;
    uint64_t m_fence_value = 0;
    std::shared_ptr<Fence> m_fence;
    std::shared_ptr<Resource> m_buffer;
    std::array<std::shared_ptr<View>, kFrameCount> m_constant_buffer_views = {};
    std::shared_ptr<Shader> m_compute_shader;
    std::shared_ptr<BindingSetLayout> m_layout;
    std::shared_ptr<Pipeline> m_pipeline;

    std::shared_ptr<Swapchain> m_swapchain;
    std::shared_ptr<Resource> m_result_texture;
    std::shared_ptr<View> m_result_texture_view;
    std::array<std::shared_ptr<BindingSet>, kFrameCount> m_binding_set = {};
    std::array<std::shared_ptr<CommandList>, kFrameCount> m_command_lists = {};
    std::array<uint64_t, kFrameCount> m_fence_values = {};
    std::array<float, kFrameCount> m_constant_data = {};
};

DispatchIndirectRenderer::DispatchIndirectRenderer(const Settings& settings)
    : m_settings(settings)
{
    m_instance = CreateInstance(m_settings.api_type);
    m_adapter = std::move(m_instance->EnumerateAdapters()[m_settings.required_gpu_index]);
    m_device = m_adapter->CreateDevice();
    m_command_queue = m_device->GetCommandQueue(CommandListType::kGraphics);
    m_fence = m_device->CreateFence(m_fence_value);

    m_buffer = m_device->CreateBuffer(MemoryType::kUpload,
                                      { .size = sizeof(float) * kFrameCount + sizeof(DispatchIndirectCommand),
                                        .usage = BindFlag::kConstantBuffer | BindFlag::kIndirectBuffer });
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        ViewDesc constant_buffer_view_desc = {
            .view_type = ViewType::kConstantBuffer,
            .dimension = ViewDimension::kBuffer,
            .offset = i * sizeof(float),
        };
        m_constant_buffer_views[i] = m_device->CreateView(m_buffer, constant_buffer_view_desc);
    }

    DispatchIndirectCommand argument_data = { 64, 64, 1 };
    m_buffer->UpdateUploadBuffer(sizeof(float) * kFrameCount, &argument_data, sizeof(argument_data));

    ShaderBlobType blob_type = m_device->GetSupportedShaderBlobType();
    std::vector<uint8_t> compute_blob = AssetLoadShaderBlob("assets/DispatchIndirect/ComputeShader.hlsl", blob_type);
    m_compute_shader = m_device->CreateShader(compute_blob, blob_type, ShaderType::kCompute);

    BindKey constant_buffer_key = m_compute_shader->GetBindKey("constant_buffer");
    BindKey result_texture_key = m_compute_shader->GetBindKey("result_texture");
    m_layout = m_device->CreateBindingSetLayout({ constant_buffer_key, result_texture_key });

    ComputePipelineDesc pipeline_desc = {
        m_device->CreateProgram({ m_compute_shader }),
        m_layout,
    };
    m_pipeline = m_device->CreateComputePipeline(pipeline_desc);
}

DispatchIndirectRenderer::~DispatchIndirectRenderer()
{
    WaitForIdle();
}

void DispatchIndirectRenderer::Init(const AppSize& app_size, WindowHandle window)
{
    m_swapchain = m_device->CreateSwapchain(window, app_size.width(), app_size.height(), kFrameCount, m_settings.vsync);

    TextureDesc result_texture_desc = {
        .type = TextureType::k2D,
        .format = m_swapchain->GetFormat(),
        .width = 512,
        .height = 512,
        .depth_or_array_layers = 1,
        .mip_levels = 1,
        .sample_count = 1,
        .usage = BindFlag::kUnorderedAccess | BindFlag::kCopySource,
    };
    m_result_texture = m_device->CreateTexture(MemoryType::kDefault, result_texture_desc);
    ViewDesc result_texture_view_desc = {
        .view_type = ViewType::kRWTexture,
        .dimension = ViewDimension::kTexture2D,
    };
    m_result_texture_view = m_device->CreateView(m_result_texture, result_texture_view_desc);

    BindKey constant_buffer_key = m_compute_shader->GetBindKey("constant_buffer");
    BindKey result_texture_key = m_compute_shader->GetBindKey("result_texture");
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        m_binding_set[i] = m_device->CreateBindingSet(m_layout);
        m_binding_set[i]->WriteBindings(
            { { constant_buffer_key, m_constant_buffer_views[i] }, { result_texture_key, m_result_texture_view } });
    }

    for (uint32_t i = 0; i < kFrameCount; ++i) {
        std::shared_ptr<Resource> back_buffer = m_swapchain->GetBackBuffer(i);

        auto& command_list = m_command_lists[i];
        command_list = m_device->CreateCommandList(CommandListType::kGraphics);
        command_list->BindPipeline(m_pipeline);
        command_list->BindBindingSet(m_binding_set[i]);
        command_list->DispatchIndirect(m_buffer, sizeof(float) * kFrameCount);
        TextureCopyRegion region = {
            .extent = { result_texture_desc.width, result_texture_desc.height, 1 },
            .dst_offset = { static_cast<int32_t>(app_size.width() / 2 - result_texture_desc.width / 2),
                            static_cast<int32_t>(app_size.height() / 2 - result_texture_desc.height / 2) },
        };
        command_list->ResourceBarrier(
            { { m_result_texture, ResourceState::kUnorderedAccess, ResourceState::kCopySource },
              { back_buffer, ResourceState::kPresent, ResourceState::kCopyDest } });
        command_list->CopyTexture(m_result_texture, back_buffer, { region });
        command_list->ResourceBarrier(
            { { back_buffer, ResourceState::kCopyDest, ResourceState::kPresent },
              { m_result_texture, ResourceState::kCopySource, ResourceState::kUnorderedAccess } });
        command_list->Close();
    }
}

void DispatchIndirectRenderer::Resize(const AppSize& app_size, WindowHandle window)
{
    WaitForIdle();
    m_swapchain.reset();
    Init(app_size, window);
}

void DispatchIndirectRenderer::Render()
{
    uint32_t frame_index = m_swapchain->NextImage(m_fence, ++m_fence_value);
    m_command_queue->Wait(m_fence, m_fence_value);
    m_fence->Wait(m_fence_values[frame_index]);

    auto now = std::chrono::high_resolution_clock::now();
    m_constant_data[frame_index] = std::chrono::duration<float>(now.time_since_epoch()).count();
    m_buffer->UpdateUploadBuffer(sizeof(m_constant_data[frame_index]) * frame_index, &m_constant_data[frame_index],
                                 sizeof(m_constant_data[frame_index]));

    m_command_queue->ExecuteCommandLists({ m_command_lists[frame_index] });
    m_command_queue->Signal(m_fence, m_fence_values[frame_index] = ++m_fence_value);
    m_swapchain->Present(m_fence, m_fence_values[frame_index]);
}

std::string_view DispatchIndirectRenderer::GetTitle() const
{
    return "DispatchIndirect";
}

const std::string& DispatchIndirectRenderer::GetGpuName() const
{
    return m_adapter->GetName();
}

const Settings& DispatchIndirectRenderer::GetSettings() const
{
    return m_settings;
}

void DispatchIndirectRenderer::WaitForIdle()
{
    m_command_queue->Signal(m_fence, ++m_fence_value);
    m_fence->Wait(m_fence_value);
}

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppLoop::Run(std::make_unique<DispatchIndirectRenderer>(settings), argc, argv);
}

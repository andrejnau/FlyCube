#include "AppLoop/AppLoop.h"
#include "AppSettings/ArgsParser.h"
#include "Instance/Instance.h"
#include "Utilities/Asset.h"
#include "Utilities/Common.h"

#include <chrono>

namespace {

constexpr uint32_t kFrameCount = 3;
constexpr uint32_t kNumThreads = 8;

} // namespace

class DispatchIndirectRenderer : public AppRenderer {
public:
    explicit DispatchIndirectRenderer(const Settings& settings);
    ~DispatchIndirectRenderer() override;

    void Init(const NativeSurface& surface, uint32_t width, uint32_t height) override;
    void Resize(const NativeSurface& surface, uint32_t width, uint32_t height) override;
    void Render() override;
    std::string_view GetTitle() const override;
    const std::string& GetGpuName() const override;
    ApiType GetApiType() const override;

private:
    void WaitForIdle();

    Settings settings_;
    std::shared_ptr<Instance> instance_;
    std::shared_ptr<Adapter> adapter_;
    std::shared_ptr<Device> device_;
    std::shared_ptr<CommandQueue> command_queue_;
    uint64_t fence_value_ = 0;
    std::shared_ptr<Fence> fence_;
    uint64_t constant_buffer_stride_ = 0;
    std::shared_ptr<Resource> buffer_;
    std::array<std::shared_ptr<View>, kFrameCount> constant_buffer_views_ = {};
    std::shared_ptr<Shader> compute_shader_;
    std::shared_ptr<BindingSetLayout> layout_;
    std::shared_ptr<Pipeline> pipeline_;

    uint32_t width_ = 0;
    uint32_t height_ = 0;
    std::shared_ptr<Swapchain> swapchain_;
    glm::uvec2 result_texture_size_;
    std::shared_ptr<Resource> result_texture_;
    std::shared_ptr<View> result_texture_view_;
    std::array<std::shared_ptr<BindingSet>, kFrameCount> binding_set_ = {};
    std::array<std::shared_ptr<CommandList>, kFrameCount> command_lists_ = {};
    std::array<uint64_t, kFrameCount> fence_values_ = {};
};

DispatchIndirectRenderer::DispatchIndirectRenderer(const Settings& settings)
    : settings_(settings)
{
    instance_ = CreateInstance(settings_.api_type);
    adapter_ = std::move(instance_->EnumerateAdapters()[settings_.required_gpu_index]);
    device_ = adapter_->CreateDevice();
    command_queue_ = device_->GetCommandQueue(CommandListType::kGraphics);
    fence_ = device_->CreateFence(fence_value_);

    constant_buffer_stride_ = Align(sizeof(float), device_->GetConstantBufferOffsetAlignment());
    buffer_ = device_->CreateBuffer(MemoryType::kUpload,
                                    { .size = constant_buffer_stride_ * kFrameCount + sizeof(DispatchIndirectCommand),
                                      .usage = BindFlag::kConstantBuffer | BindFlag::kIndirectBuffer });
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        ViewDesc constant_buffer_view_desc = {
            .view_type = ViewType::kConstantBuffer,
            .dimension = ViewDimension::kBuffer,
            .offset = i * constant_buffer_stride_,
        };
        constant_buffer_views_[i] = device_->CreateView(buffer_, constant_buffer_view_desc);
    }

    ShaderBlobType blob_type = device_->GetSupportedShaderBlobType();
    std::vector<uint8_t> compute_blob = AssetLoadShaderBlob("assets/DispatchIndirect/ComputeShader.hlsl", blob_type);
    compute_shader_ = device_->CreateShader(compute_blob, blob_type, ShaderType::kCompute);

    BindKey constant_buffer_key = compute_shader_->GetBindKey("constant_buffer");
    BindKey result_texture_key = compute_shader_->GetBindKey("result_texture");
    layout_ = device_->CreateBindingSetLayout({ .bind_keys = { constant_buffer_key, result_texture_key } });

    ComputePipelineDesc pipeline_desc = {
        .shader = compute_shader_,
        .layout = layout_,
    };
    pipeline_ = device_->CreateComputePipeline(pipeline_desc);
}

DispatchIndirectRenderer::~DispatchIndirectRenderer()
{
    WaitForIdle();
}

void DispatchIndirectRenderer::Init(const NativeSurface& surface, uint32_t width, uint32_t height)
{
    width_ = width;
    height_ = height;
    swapchain_ = device_->CreateSwapchain(surface, width, height, kFrameCount, settings_.vsync);

    result_texture_size_ = glm::uvec2(std::min(512u, width), std::min(512u, height));
    TextureDesc result_texture_desc = {
        .type = TextureType::k2D,
        .format = swapchain_->GetFormat(),
        .width = result_texture_size_.x,
        .height = result_texture_size_.y,
        .depth_or_array_layers = 1,
        .mip_levels = 1,
        .sample_count = 1,
        .usage = BindFlag::kUnorderedAccess | BindFlag::kCopySource,
    };
    result_texture_ = device_->CreateTexture(MemoryType::kDefault, result_texture_desc);
    ViewDesc result_texture_view_desc = {
        .view_type = ViewType::kRWTexture,
        .dimension = ViewDimension::kTexture2D,
    };
    result_texture_view_ = device_->CreateView(result_texture_, result_texture_view_desc);

    DispatchIndirectCommand argument_data = { (result_texture_size_.x + kNumThreads - 1) / kNumThreads,
                                              (result_texture_size_.y + kNumThreads - 1) / kNumThreads, 1 };
    buffer_->UpdateUploadBuffer(constant_buffer_stride_ * kFrameCount, &argument_data, sizeof(argument_data));

    BindKey constant_buffer_key = compute_shader_->GetBindKey("constant_buffer");
    BindKey result_texture_key = compute_shader_->GetBindKey("result_texture");
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        binding_set_[i] = device_->CreateBindingSet(layout_);
        binding_set_[i]->WriteBindings({ .bindings = { { constant_buffer_key, constant_buffer_views_[i] },
                                                       { result_texture_key, result_texture_view_ } } });
        command_lists_[i] = device_->CreateCommandList(CommandListType::kGraphics);
    }
}

void DispatchIndirectRenderer::Resize(const NativeSurface& surface, uint32_t width, uint32_t height)
{
    WaitForIdle();
    swapchain_.reset();
    Init(surface, width, height);
}

void DispatchIndirectRenderer::Render()
{
    uint32_t frame_index = swapchain_->NextImage(fence_, ++fence_value_);
    command_queue_->Wait(fence_, fence_value_);
    fence_->Wait(fence_values_[frame_index]);
    std::shared_ptr<Resource> back_buffer = swapchain_->GetBackBuffer(frame_index);

    auto& command_list = command_lists_[frame_index];
    command_list->Reset();
    command_list->BindPipeline(pipeline_);
    command_list->BindBindingSet(binding_set_[frame_index]);
    command_list->DispatchIndirect(buffer_, constant_buffer_stride_ * kFrameCount);
    TextureCopyRegion region = {
        .extent = { result_texture_size_.x, result_texture_size_.y, 1 },
        .dst_offset = { (width_ - result_texture_size_.x) / 2, (height_ - result_texture_size_.y) / 2 },
    };
    command_list->ResourceBarrier({ { result_texture_, ResourceState::kUnorderedAccess, ResourceState::kCopySource },
                                    { back_buffer, ResourceState::kPresent, ResourceState::kCopyDest } });
    command_list->CopyTexture(result_texture_, back_buffer, { region });
    command_list->ResourceBarrier({ { back_buffer, ResourceState::kCopyDest, ResourceState::kPresent },
                                    { result_texture_, ResourceState::kCopySource, ResourceState::kUnorderedAccess } });
    command_list->Close();

    auto now = std::chrono::high_resolution_clock::now();
    float constant_data = std::chrono::duration<float>(now.time_since_epoch()).count();
    buffer_->UpdateUploadBuffer(constant_buffer_stride_ * frame_index, &constant_data, sizeof(constant_data));

    command_queue_->ExecuteCommandLists({ command_list });
    command_queue_->Signal(fence_, fence_values_[frame_index] = ++fence_value_);
    swapchain_->Present(fence_, fence_values_[frame_index]);
}

std::string_view DispatchIndirectRenderer::GetTitle() const
{
    return "DispatchIndirect";
}

const std::string& DispatchIndirectRenderer::GetGpuName() const
{
    return adapter_->GetName();
}

ApiType DispatchIndirectRenderer::GetApiType() const
{
    return settings_.api_type;
}

void DispatchIndirectRenderer::WaitForIdle()
{
    command_queue_->Signal(fence_, ++fence_value_);
    fence_->Wait(fence_value_);
}

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppLoop::Run(std::make_unique<DispatchIndirectRenderer>(settings), argc, argv);
}

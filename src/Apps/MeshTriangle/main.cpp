#include "AppLoop/AppLoop.h"
#include "AppSettings/ArgsParser.h"
#include "Instance/Instance.h"
#include "Utilities/Asset.h"
#include "Utilities/Check.h"

namespace {

constexpr uint32_t kFrameCount = 3;

} // namespace

class MeshTriangleRenderer : public AppRenderer {
public:
    explicit MeshTriangleRenderer(const Settings& settings);
    ~MeshTriangleRenderer() override;

    void Init(const NativeSurface& surface, uint32_t width, uint32_t height) override;
    void Resize(const NativeSurface& surface, uint32_t width, uint32_t height) override;
    void Render() override;
    std::string_view GetTitle() const override;
    const std::string& GetGpuName() const override;
    const Settings& GetSettings() const override;

private:
    void WaitForIdle();

    Settings settings_;
    std::shared_ptr<Instance> instance_;
    std::shared_ptr<Adapter> adapter_;
    std::shared_ptr<Device> device_;
    std::shared_ptr<CommandQueue> command_queue_;
    uint64_t fence_value_ = 0;
    std::shared_ptr<Fence> fence_;
    std::shared_ptr<Shader> mesh_shader_;
    std::shared_ptr<Shader> pixel_shader_;
    std::shared_ptr<BindingSetLayout> layout_;

    std::shared_ptr<Swapchain> swapchain_;
    std::shared_ptr<Pipeline> pipeline_;
    std::array<std::shared_ptr<View>, kFrameCount> back_buffer_views_ = {};
    std::array<std::shared_ptr<CommandList>, kFrameCount> command_lists_ = {};
    std::array<uint64_t, kFrameCount> fence_values_ = {};
};

MeshTriangleRenderer::MeshTriangleRenderer(const Settings& settings)
    : settings_(settings)
{
    instance_ = CreateInstance(settings_.api_type);
    adapter_ = std::move(instance_->EnumerateAdapters()[settings_.required_gpu_index]);
    device_ = adapter_->CreateDevice();
    CHECK(device_->IsMeshShadingSupported(), "Mesh Shading is not supported");
    command_queue_ = device_->GetCommandQueue(CommandListType::kGraphics);
    fence_ = device_->CreateFence(fence_value_);

    ShaderBlobType blob_type = device_->GetSupportedShaderBlobType();
    std::vector<uint8_t> mesh_blob = AssetLoadShaderBlob("assets/MeshTriangle/MeshShader.hlsl", blob_type);
    std::vector<uint8_t> pixel_blob = AssetLoadShaderBlob("assets/MeshTriangle/PixelShader.hlsl", blob_type);
    mesh_shader_ = device_->CreateShader(mesh_blob, blob_type, ShaderType::kMesh);
    pixel_shader_ = device_->CreateShader(pixel_blob, blob_type, ShaderType::kPixel);
    layout_ = device_->CreateBindingSetLayout({});
}

MeshTriangleRenderer::~MeshTriangleRenderer()
{
    WaitForIdle();
}

void MeshTriangleRenderer::Init(const NativeSurface& surface, uint32_t width, uint32_t height)
{
    swapchain_ = device_->CreateSwapchain(surface, width, height, kFrameCount, settings_.vsync);

    GraphicsPipelineDesc pipeline_desc = {
        .shaders = { mesh_shader_, pixel_shader_ },
        .layout = layout_,
        .color_formats = { swapchain_->GetFormat() },
    };
    pipeline_ = device_->CreateGraphicsPipeline(pipeline_desc);

    for (uint32_t i = 0; i < kFrameCount; ++i) {
        std::shared_ptr<Resource> back_buffer = swapchain_->GetBackBuffer(i);
        ViewDesc back_buffer_view_desc = {
            .view_type = ViewType::kRenderTarget,
            .dimension = ViewDimension::kTexture2D,
        };
        back_buffer_views_[i] = device_->CreateView(back_buffer, back_buffer_view_desc);

        auto& command_list = command_lists_[i];
        command_list = device_->CreateCommandList(CommandListType::kGraphics);
        command_list->BindPipeline(pipeline_);
        command_list->SetViewport(0, 0, width, height, 0.0, 1.0);
        command_list->SetScissorRect(0, 0, width, height);
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kRenderTarget } });
        RenderPassDesc render_pass_desc = {
            .render_area = { 0, 0, width, height },
            .colors = { { .view = back_buffer_views_[i],
                          .load_op = RenderPassLoadOp::kClear,
                          .store_op = RenderPassStoreOp::kStore,
                          .clear_value = { 0.0, 0.2, 0.4, 1.0 } } },
        };
        command_list->BeginRenderPass(render_pass_desc);
        command_list->DispatchMesh(1, 1, 1);
        command_list->EndRenderPass();
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kRenderTarget, ResourceState::kPresent } });
        command_list->Close();
    }
}

void MeshTriangleRenderer::Resize(const NativeSurface& surface, uint32_t width, uint32_t height)
{
    WaitForIdle();
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        back_buffer_views_[i].reset();
    }
    swapchain_.reset();
    Init(surface, width, height);
}

void MeshTriangleRenderer::Render()
{
    uint32_t frame_index = swapchain_->NextImage(fence_, ++fence_value_);
    command_queue_->Wait(fence_, fence_value_);
    fence_->Wait(fence_values_[frame_index]);
    command_queue_->ExecuteCommandLists({ command_lists_[frame_index] });
    command_queue_->Signal(fence_, fence_values_[frame_index] = ++fence_value_);
    swapchain_->Present(fence_, fence_values_[frame_index]);
}

std::string_view MeshTriangleRenderer::GetTitle() const
{
    return "MeshTriangle";
}

const std::string& MeshTriangleRenderer::GetGpuName() const
{
    return adapter_->GetName();
}

const Settings& MeshTriangleRenderer::GetSettings() const
{
    return settings_;
}

void MeshTriangleRenderer::WaitForIdle()
{
    command_queue_->Signal(fence_, ++fence_value_);
    fence_->Wait(fence_value_);
}

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppLoop::Run(std::make_unique<MeshTriangleRenderer>(settings), argc, argv);
}

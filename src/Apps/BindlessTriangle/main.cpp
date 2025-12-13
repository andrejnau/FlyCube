#include "AppLoop/AppLoop.h"
#include "AppSettings/ArgsParser.h"
#include "Instance/Instance.h"
#include "Utilities/Asset.h"
#include "Utilities/Check.h"

namespace {

constexpr uint32_t kFrameCount = 3;
constexpr bool kUseBindlessTypedViewPool = true;

} // namespace

class BindlessTriangleRenderer : public AppRenderer {
public:
    explicit BindlessTriangleRenderer(const Settings& settings);
    ~BindlessTriangleRenderer() override;

    void Init(const AppSize& app_size, const NativeSurface& surface) override;
    void Resize(const AppSize& app_size, const NativeSurface& surface) override;
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
    std::shared_ptr<Resource> index_buffer_;
    std::shared_ptr<View> index_buffer_view_;
    std::shared_ptr<Resource> vertex_buffer_;
    std::shared_ptr<View> vertex_buffer_view_;
    std::shared_ptr<BindlessTypedViewPool> bindless_view_pool_;
    std::shared_ptr<Resource> vertex_constant_buffer_;
    std::shared_ptr<View> vertex_constant_buffer_view_;
    std::shared_ptr<Resource> pixel_constant_buffer_;
    std::shared_ptr<View> pixel_constant_buffer_view_;
    std::shared_ptr<Shader> vertex_shader_;
    std::shared_ptr<Shader> pixel_shader_;
    std::shared_ptr<BindingSetLayout> layout_;
    std::shared_ptr<BindingSet> binding_set_;

    std::shared_ptr<Swapchain> swapchain_;
    std::shared_ptr<Pipeline> pipeline_;
    std::array<std::shared_ptr<View>, kFrameCount> back_buffer_views_ = {};
    std::array<std::shared_ptr<CommandList>, kFrameCount> command_lists_ = {};
    std::array<uint64_t, kFrameCount> fence_values_ = {};
};

BindlessTriangleRenderer::BindlessTriangleRenderer(const Settings& settings)
    : settings_(settings)
{
    instance_ = CreateInstance(settings_.api_type);
    adapter_ = std::move(instance_->EnumerateAdapters()[settings_.required_gpu_index]);
    device_ = adapter_->CreateDevice();
    CHECK(device_->IsBindlessSupported(), "Bindless is not supported");
    command_queue_ = device_->GetCommandQueue(CommandListType::kGraphics);
    fence_ = device_->CreateFence(fence_value_);

    std::vector<uint32_t> index_data = { 0, 1, 2 };
    index_buffer_ = device_->CreateBuffer(MemoryType::kUpload, { .size = sizeof(index_data.front()) * index_data.size(),
                                                                 .usage = BindFlag::kShaderResource });
    index_buffer_->UpdateUploadBuffer(0, index_data.data(), sizeof(index_data.front()) * index_data.size());
    ViewDesc index_buffer_view_desc = {
        .view_type = ViewType::kStructuredBuffer,
        .dimension = ViewDimension::kBuffer,
        .structure_stride = sizeof(index_data.front()),
        .bindless = !kUseBindlessTypedViewPool,
    };
    index_buffer_view_ = device_->CreateView(index_buffer_, index_buffer_view_desc);

    std::vector<glm::vec3> vertex_data = {
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3(0.0, 0.5, 0.0),
        glm::vec3(0.5, -0.5, 0.0),
    };
    vertex_buffer_ = device_->CreateBuffer(
        MemoryType::kUpload,
        { .size = sizeof(vertex_data.front()) * vertex_data.size(), .usage = BindFlag::kShaderResource });
    vertex_buffer_->UpdateUploadBuffer(0, vertex_data.data(), sizeof(vertex_data.front()) * vertex_data.size());
    ViewDesc vertex_buffer_view_desc = {
        .view_type = ViewType::kStructuredBuffer,
        .dimension = ViewDimension::kBuffer,
        .structure_stride = sizeof(vertex_data.front()),
        .bindless = !kUseBindlessTypedViewPool,
    };
    vertex_buffer_view_ = device_->CreateView(vertex_buffer_, vertex_buffer_view_desc);

    std::pair<uint32_t, uint32_t> vertex_constant_data = {};
    if (kUseBindlessTypedViewPool) {
        bindless_view_pool_ = device_->CreateBindlessTypedViewPool(ViewType::kStructuredBuffer, 2);
        bindless_view_pool_->WriteView(0, index_buffer_view_);
        bindless_view_pool_->WriteView(1, vertex_buffer_view_);
        vertex_constant_data = { bindless_view_pool_->GetBaseDescriptorId() + 0,
                                 bindless_view_pool_->GetBaseDescriptorId() + 1 };
    } else {
        vertex_constant_data = { index_buffer_view_->GetDescriptorId(), vertex_buffer_view_->GetDescriptorId() };
    }
    vertex_constant_buffer_ = device_->CreateBuffer(
        MemoryType::kUpload, { .size = sizeof(vertex_constant_data), .usage = BindFlag::kConstantBuffer });
    vertex_constant_buffer_->UpdateUploadBuffer(0, &vertex_constant_data, sizeof(vertex_constant_data));
    ViewDesc vertex_constant_buffer_view_desc = {
        .view_type = ViewType::kConstantBuffer,
        .dimension = ViewDimension::kBuffer,
    };
    vertex_constant_buffer_view_ = device_->CreateView(vertex_constant_buffer_, vertex_constant_buffer_view_desc);

    glm::vec4 pixel_constant_data = glm::vec4(1.0, 0.0, 0.0, 1.0);
    pixel_constant_buffer_ = device_->CreateBuffer(
        MemoryType::kUpload, { .size = sizeof(pixel_constant_data), .usage = BindFlag::kConstantBuffer });
    pixel_constant_buffer_->UpdateUploadBuffer(0, &pixel_constant_data, sizeof(pixel_constant_data));
    ViewDesc pixel_constant_buffer_view_desc = {
        .view_type = ViewType::kConstantBuffer,
        .dimension = ViewDimension::kBuffer,
    };
    pixel_constant_buffer_view_ = device_->CreateView(pixel_constant_buffer_, pixel_constant_buffer_view_desc);

    ShaderBlobType blob_type = device_->GetSupportedShaderBlobType();
    std::vector<uint8_t> vertex_blob = AssetLoadShaderBlob("assets/BindlessTriangle/VertexShader.hlsl", blob_type);
    std::vector<uint8_t> pixel_blob = AssetLoadShaderBlob("assets/BindlessTriangle/PixelShader.hlsl", blob_type);
    vertex_shader_ = device_->CreateShader(vertex_blob, blob_type, ShaderType::kVertex);
    pixel_shader_ = device_->CreateShader(pixel_blob, blob_type, ShaderType::kPixel);

    BindKey vertex_constant_buffer_key = {
        .shader_type = ShaderType::kVertex,
        .view_type = ViewType::kConstantBuffer,
        .slot = 0,
        .space = 0,
        .count = 1,
    };
    BindKey vertex_structured_buffers_uint_key = {
        .shader_type = ShaderType::kVertex,
        .view_type = ViewType::kStructuredBuffer,
        .slot = 0,
        .space = 1,
        .count = kBindlessCount,
    };
    BindKey vertex_structured_buffers_float3_key = {
        .shader_type = ShaderType::kVertex,
        .view_type = ViewType::kStructuredBuffer,
        .slot = 0,
        .space = 2,
        .count = kBindlessCount,
    };
    BindKey pixel_constant_buffer_key = {
        .shader_type = ShaderType::kPixel,
        .view_type = ViewType::kConstantBuffer,
        .slot = 1,
        .space = 0,
        .count = 1,
    };
    layout_ = device_->CreateBindingSetLayout(
        { .bind_keys = { vertex_constant_buffer_key, vertex_structured_buffers_uint_key,
                         vertex_structured_buffers_float3_key, pixel_constant_buffer_key } });
    binding_set_ = device_->CreateBindingSet(layout_);
    binding_set_->WriteBindings({ .bindings = { { vertex_constant_buffer_key, vertex_constant_buffer_view_ },
                                                { pixel_constant_buffer_key, pixel_constant_buffer_view_ } } });
}

BindlessTriangleRenderer::~BindlessTriangleRenderer()
{
    WaitForIdle();
}

void BindlessTriangleRenderer::Init(const AppSize& app_size, const NativeSurface& surface)
{
    swapchain_ = device_->CreateSwapchain(surface, app_size.width(), app_size.height(), kFrameCount, settings_.vsync);

    GraphicsPipelineDesc pipeline_desc = {
        .shaders = { vertex_shader_, pixel_shader_ },
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
        command_list->BindBindingSet(binding_set_);
        command_list->SetViewport(0, 0, app_size.width(), app_size.height(), 0.0, 1.0);
        command_list->SetScissorRect(0, 0, app_size.width(), app_size.height());
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kRenderTarget } });
        RenderPassDesc render_pass_desc = {
            .render_area = { 0, 0, app_size.width(), app_size.height() },
            .colors = { { .view = back_buffer_views_[i],
                          .load_op = RenderPassLoadOp::kClear,
                          .store_op = RenderPassStoreOp::kStore,
                          .clear_value = { 0.0, 0.2, 0.4, 1.0 } } },
        };
        command_list->BeginRenderPass(render_pass_desc);
        command_list->Draw(3, 1, 0, 0);
        command_list->EndRenderPass();
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kRenderTarget, ResourceState::kPresent } });
        command_list->Close();
    }
}

void BindlessTriangleRenderer::Resize(const AppSize& app_size, const NativeSurface& surface)
{
    WaitForIdle();
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        back_buffer_views_[i].reset();
    }
    swapchain_.reset();
    Init(app_size, surface);
}

void BindlessTriangleRenderer::Render()
{
    uint32_t frame_index = swapchain_->NextImage(fence_, ++fence_value_);
    command_queue_->Wait(fence_, fence_value_);
    fence_->Wait(fence_values_[frame_index]);
    command_queue_->ExecuteCommandLists({ command_lists_[frame_index] });
    command_queue_->Signal(fence_, fence_values_[frame_index] = ++fence_value_);
    swapchain_->Present(fence_, fence_values_[frame_index]);
}

std::string_view BindlessTriangleRenderer::GetTitle() const
{
    return "BindlessTriangle";
}

const std::string& BindlessTriangleRenderer::GetGpuName() const
{
    return adapter_->GetName();
}

const Settings& BindlessTriangleRenderer::GetSettings() const
{
    return settings_;
}

void BindlessTriangleRenderer::WaitForIdle()
{
    command_queue_->Signal(fence_, ++fence_value_);
    fence_->Wait(fence_value_);
}

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppLoop::Run(std::make_unique<BindlessTriangleRenderer>(settings), argc, argv);
}

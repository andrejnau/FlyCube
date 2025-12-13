#include "AppLoop/AppLoop.h"
#include "AppSettings/ArgsParser.h"
#include "Instance/Instance.h"
#include "RenderUtils/ModelLoader.h"
#include "RenderUtils/RenderModel.h"
#include "Utilities/Asset.h"

namespace {

constexpr uint32_t kPositions = 0;
constexpr uint32_t kTexcoords = 1;
constexpr uint32_t kFrameCount = 3;

glm::mat4 GetViewMatrix()
{
    glm::vec3 eye = glm::vec3(0.0, 0.0, 3.5);
    glm::vec3 center = glm::vec3(0.0, 0.0, 0.0);
    glm::vec3 up = glm::vec3(0.0, 1.0, 0.0);
    return glm::lookAt(eye, center, up);
}

glm::mat4 GetProjectionMatrix(uint32_t width, uint32_t height)
{
    return glm::perspective(glm::radians(45.0f), static_cast<float>(width) / height,
                            /*zNear=*/0.1f, /*zFar=*/128.0f);
}

} // namespace

class DepthStencilReadRenderer : public AppRenderer {
public:
    explicit DepthStencilReadRenderer(const Settings& settings);
    ~DepthStencilReadRenderer() override;

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
    RenderModel render_model_;
    std::shared_ptr<Resource> fullscreen_triangle_vertex_buffer_;
    std::shared_ptr<Resource> depth_stencil_pass_constant_buffer_;
    std::vector<std::shared_ptr<View>> depth_stencil_pass_constant_buffer_views_;
    std::shared_ptr<Resource> vertex_constant_buffer_;
    std::shared_ptr<View> vertex_constant_buffer_view_;
    std::shared_ptr<Resource> pixel_constant_buffer_;
    std::shared_ptr<View> pixel_constant_buffer_view_;
    std::shared_ptr<Shader> vertex_shader_;
    std::shared_ptr<Shader> pixel_shader_;
    std::shared_ptr<BindingSetLayout> layout_;
    std::vector<std::shared_ptr<BindingSet>> depth_stencil_pass_binding_sets_;

    std::shared_ptr<Swapchain> swapchain_;
    std::shared_ptr<Resource> depth_stencil_texture_;
    std::shared_ptr<View> depth_stencil_view_;
    std::shared_ptr<View> depth_read_view_;
    std::shared_ptr<View> stencil_read_view_;
    std::shared_ptr<Pipeline> depth_stencil_pass_pipeline_;
    std::shared_ptr<Pipeline> pipeline_;
    std::shared_ptr<BindingSet> binding_set_;
    std::array<std::shared_ptr<View>, kFrameCount> back_buffer_views_ = {};
    std::array<std::shared_ptr<CommandList>, kFrameCount> command_lists_ = {};
    std::array<uint64_t, kFrameCount> fence_values_ = {};
};

DepthStencilReadRenderer::DepthStencilReadRenderer(const Settings& settings)
    : settings_(settings)
{
    instance_ = CreateInstance(settings_.api_type);
    adapter_ = std::move(instance_->EnumerateAdapters()[settings_.required_gpu_index]);
    device_ = adapter_->CreateDevice();
    command_queue_ = device_->GetCommandQueue(CommandListType::kGraphics);
    fence_ = device_->CreateFence(fence_value_);

    std::unique_ptr<Model> model = LoadModel("assets/DepthStencilRead/DamagedHelmet.gltf");
    render_model_ = RenderModel(device_, command_queue_, std::move(model));

    struct VertexLayout {
        glm::vec3 position;
        glm::vec2 texcoord;
    };
    auto vertex_data = std::to_array<VertexLayout>({
        { glm::vec3(-1.0, 1.0, 0.0), glm::vec2(0.0, 0.0) },
        { glm::vec3(3.0, 1.0, 0.0), glm::vec2(2.0, 0.0) },
        { glm::vec3(-1.0, -3.0, 0.0), glm::vec2(0.0, 2.0) },
    });
    fullscreen_triangle_vertex_buffer_ = device_->CreateBuffer(
        MemoryType::kUpload,
        { .size = sizeof(vertex_data.front()) * vertex_data.size(), .usage = BindFlag::kVertexBuffer });
    fullscreen_triangle_vertex_buffer_->UpdateUploadBuffer(0, vertex_data.data(),
                                                           sizeof(vertex_data.front()) * vertex_data.size());

    depth_stencil_pass_constant_buffer_ = device_->CreateBuffer(
        MemoryType::kUpload,
        { .size = sizeof(glm::mat4) * render_model_.GetMeshCount(), .usage = BindFlag::kConstantBuffer });
    depth_stencil_pass_constant_buffer_views_.resize(render_model_.GetMeshCount());
    for (size_t i = 0; i < render_model_.GetMeshCount(); ++i) {
        ViewDesc depth_stencil_pass_constant_buffer_view_desc = {
            .view_type = ViewType::kConstantBuffer,
            .dimension = ViewDimension::kBuffer,
            .offset = i * sizeof(glm::mat4),
        };
        depth_stencil_pass_constant_buffer_views_[i] =
            device_->CreateView(depth_stencil_pass_constant_buffer_, depth_stencil_pass_constant_buffer_view_desc);
    }

    glm::mat4 vertex_constant_data = glm::transpose(glm::mat4(1.0));
    vertex_constant_buffer_ = device_->CreateBuffer(
        MemoryType::kUpload, { .size = sizeof(vertex_constant_data), .usage = BindFlag::kConstantBuffer });
    ViewDesc vertex_constant_buffer_view_desc = {
        .view_type = ViewType::kConstantBuffer,
        .dimension = ViewDimension::kBuffer,
    };
    vertex_constant_buffer_view_ = device_->CreateView(vertex_constant_buffer_, vertex_constant_buffer_view_desc);
    vertex_constant_buffer_->UpdateUploadBuffer(0, &vertex_constant_data, sizeof(vertex_constant_data));

    pixel_constant_buffer_ =
        device_->CreateBuffer(MemoryType::kUpload, { .size = sizeof(glm::uvec2), .usage = BindFlag::kConstantBuffer });
    ViewDesc pixel_constant_buffer_view_desc = {
        .view_type = ViewType::kConstantBuffer,
        .dimension = ViewDimension::kBuffer,
    };
    pixel_constant_buffer_view_ = device_->CreateView(pixel_constant_buffer_, pixel_constant_buffer_view_desc);

    ShaderBlobType blob_type = device_->GetSupportedShaderBlobType();
    std::vector<uint8_t> vertex_blob = AssetLoadShaderBlob("assets/DepthStencilRead/VertexShader.hlsl", blob_type);
    std::vector<uint8_t> pixel_blob = AssetLoadShaderBlob("assets/DepthStencilRead/PixelShader.hlsl", blob_type);
    vertex_shader_ = device_->CreateShader(vertex_blob, blob_type, ShaderType::kVertex);
    pixel_shader_ = device_->CreateShader(pixel_blob, blob_type, ShaderType::kPixel);

    BindKey vertex_constant_buffer_key = vertex_shader_->GetBindKey("constant_buffer");
    BindKey pixel_constant_buffer_key = pixel_shader_->GetBindKey("constant_buffer");
    BindKey pixel_depth_buffer_key = pixel_shader_->GetBindKey("depth_buffer");
    BindKey pixel_stencil_buffer_key = pixel_shader_->GetBindKey("stencil_buffer");
    layout_ = device_->CreateBindingSetLayout({ .bind_keys = { vertex_constant_buffer_key, pixel_constant_buffer_key,
                                                               pixel_depth_buffer_key, pixel_stencil_buffer_key } });

    depth_stencil_pass_binding_sets_.resize(render_model_.GetMeshCount());
    for (size_t i = 0; i < render_model_.GetMeshCount(); ++i) {
        depth_stencil_pass_binding_sets_[i] = device_->CreateBindingSet(layout_);
        depth_stencil_pass_binding_sets_[i]->WriteBindings(
            { .bindings = { { vertex_constant_buffer_key, depth_stencil_pass_constant_buffer_views_[i] } } });
    }
}

DepthStencilReadRenderer::~DepthStencilReadRenderer()
{
    WaitForIdle();
}

void DepthStencilReadRenderer::Init(const NativeSurface& surface, uint32_t width, uint32_t height)
{
    swapchain_ = device_->CreateSwapchain(surface, width, height, kFrameCount, settings_.vsync);

    glm::mat4 view = GetViewMatrix();
    glm::uvec2 depth_stencil_size = glm::uvec2(width / 2, height);
    glm::mat4 projection = GetProjectionMatrix(depth_stencil_size.x, depth_stencil_size.y);

    std::vector<glm::mat4> depth_stencil_pass_constant_data(render_model_.GetMeshCount());
    for (size_t i = 0; i < render_model_.GetMeshCount(); ++i) {
        depth_stencil_pass_constant_data[i] = glm::transpose(projection * view * render_model_.GetMesh(i).matrix);
    }
    depth_stencil_pass_constant_buffer_->UpdateUploadBuffer(
        0, depth_stencil_pass_constant_data.data(),
        sizeof(depth_stencil_pass_constant_data.front()) * depth_stencil_pass_constant_data.size());

    glm::uvec2 pixel_constant_data = glm::uvec2(width, height);
    pixel_constant_buffer_->UpdateUploadBuffer(0, &pixel_constant_data, sizeof(pixel_constant_data));

    TextureDesc depth_stencil_texture_desc = {
        .type = TextureType::k2D,
        .format = gli::format::FORMAT_D32_SFLOAT_S8_UINT_PACK64,
        .width = depth_stencil_size.x,
        .height = depth_stencil_size.y,
        .depth_or_array_layers = 1,
        .mip_levels = 1,
        .sample_count = 1,
        .usage = BindFlag::kDepthStencil | BindFlag::kShaderResource,
    };
    depth_stencil_texture_ = device_->CreateTexture(MemoryType::kDefault, depth_stencil_texture_desc);
    ViewDesc depth_stencil_view_desc = {
        .view_type = ViewType::kDepthStencil,
        .dimension = ViewDimension::kTexture2D,
    };
    depth_stencil_view_ = device_->CreateView(depth_stencil_texture_, depth_stencil_view_desc);
    ViewDesc depth_read_view_desc = {
        .view_type = ViewType::kTexture,
        .dimension = ViewDimension::kTexture2D,
        .plane_slice = 0,
    };
    depth_read_view_ = device_->CreateView(depth_stencil_texture_, depth_read_view_desc);
    ViewDesc stencil_read_view_desc = {
        .view_type = ViewType::kTexture,
        .dimension = ViewDimension::kTexture2D,
        .plane_slice = 1,
    };
    stencil_read_view_ = device_->CreateView(depth_stencil_texture_, stencil_read_view_desc);

    DepthStencilDesc depth_stencil_pass_depth_stencil_desc = {
        .stencil_enable = true,
        .front_face = { .depth_fail_op = StencilOp::kIncr, .pass_op = StencilOp::kIncr },
        .back_face = { .depth_fail_op = StencilOp::kIncr, .pass_op = StencilOp::kIncr },
    };

    GraphicsPipelineDesc depth_stencil_pass_pipeline_desc = {
        .shaders = { vertex_shader_ },
        .layout = layout_,
        .input = { { kPositions, "POSITION", gli::FORMAT_RGB32_SFLOAT_PACK32, sizeof(glm::vec3) },
                   { kTexcoords, "TEXCOORD", gli::FORMAT_RG32_SFLOAT_PACK32, sizeof(glm::vec2) } },
        .depth_stencil_format = depth_stencil_texture_->GetFormat(),
        .depth_stencil_desc = depth_stencil_pass_depth_stencil_desc,
    };
    depth_stencil_pass_pipeline_ = device_->CreateGraphicsPipeline(depth_stencil_pass_pipeline_desc);

    DepthStencilDesc depth_stencil_desc = {
        .depth_test_enable = false,
        .depth_write_enable = false,
        .depth_func = ComparisonFunc::kAlways,
    };

    GraphicsPipelineDesc pipeline_desc = {
        .shaders = { vertex_shader_, pixel_shader_ },
        .layout = layout_,
        .input = { { 0, "POSITION", gli::FORMAT_RGB32_SFLOAT_PACK32, sizeof(glm::vec3) + sizeof(glm::vec2), 0 },
                   { 0, "TEXCOORD", gli::FORMAT_RG32_SFLOAT_PACK32, sizeof(glm::vec3) + sizeof(glm::vec2),
                     sizeof(glm::vec3) } },
        .color_formats = { swapchain_->GetFormat() },
        .depth_stencil_desc = depth_stencil_desc,
    };
    pipeline_ = device_->CreateGraphicsPipeline(pipeline_desc);

    BindKey vertex_constant_buffer_key = vertex_shader_->GetBindKey("constant_buffer");
    BindKey pixel_constant_buffer_key = pixel_shader_->GetBindKey("constant_buffer");
    BindKey pixel_depth_buffer_key = pixel_shader_->GetBindKey("depth_buffer");
    BindKey pixel_stencil_buffer_key = pixel_shader_->GetBindKey("stencil_buffer");

    binding_set_ = device_->CreateBindingSet(layout_);
    binding_set_->WriteBindings({ .bindings = { { vertex_constant_buffer_key, vertex_constant_buffer_view_ },
                                                { pixel_constant_buffer_key, pixel_constant_buffer_view_ },
                                                { pixel_depth_buffer_key, depth_read_view_ },
                                                { pixel_stencil_buffer_key, stencil_read_view_ } } });

    for (uint32_t i = 0; i < kFrameCount; ++i) {
        std::shared_ptr<Resource> back_buffer = swapchain_->GetBackBuffer(i);
        ViewDesc back_buffer_view_desc = {
            .view_type = ViewType::kRenderTarget,
            .dimension = ViewDimension::kTexture2D,
        };
        back_buffer_views_[i] = device_->CreateView(back_buffer, back_buffer_view_desc);

        auto& command_list = command_lists_[i];
        command_list = device_->CreateCommandList(CommandListType::kGraphics);
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kRenderTarget } });

        command_list->BindPipeline(depth_stencil_pass_pipeline_);
        RenderPassDesc depth_stencil_pass_render_pass_desc = {
            .render_area = { 0, 0, depth_stencil_size.x, depth_stencil_size.y },
            .depth = { .load_op = RenderPassLoadOp::kClear, .store_op = RenderPassStoreOp::kStore, .clear_value = 1.0 },
            .stencil = { .load_op = RenderPassLoadOp::kClear, .store_op = RenderPassStoreOp::kStore, .clear_value = 0 },
            .depth_stencil_view = depth_stencil_view_,
        };
        command_list->BeginRenderPass(depth_stencil_pass_render_pass_desc);
        command_list->SetViewport(0, 0, depth_stencil_size.x, depth_stencil_size.y, 0.0, 1.0);
        command_list->SetScissorRect(0, 0, depth_stencil_size.x, depth_stencil_size.y);
        for (size_t j = 0; j < render_model_.GetMeshCount(); ++j) {
            const auto& mesh = render_model_.GetMesh(j);
            command_list->BindBindingSet(depth_stencil_pass_binding_sets_[j]);
            command_list->IASetIndexBuffer(mesh.indices.buffer, mesh.indices.offset, mesh.index_format);
            command_list->IASetVertexBuffer(kPositions, mesh.positions.buffer, mesh.positions.offset);
            command_list->IASetVertexBuffer(kTexcoords, mesh.texcoords.buffer, mesh.texcoords.offset);
            command_list->DrawIndexed(mesh.index_count, 1, 0, 0, 0);
        }
        command_list->EndRenderPass();

        command_list->BindPipeline(pipeline_);
        RenderPassDesc render_pass_desc = {
            .render_area = { 0, 0, width, height },
            .colors = { { .view = back_buffer_views_[i],
                          .load_op = RenderPassLoadOp::kDontCare,
                          .store_op = RenderPassStoreOp::kStore } },
        };
        command_list->BeginRenderPass(render_pass_desc);
        command_list->SetViewport(0, 0, width, height, 0.0, 1.0);
        command_list->SetScissorRect(0, 0, width, height);
        command_list->BindBindingSet(binding_set_);
        command_list->IASetVertexBuffer(0, fullscreen_triangle_vertex_buffer_, 0);
        command_list->Draw(3, 1, 0, 0);
        command_list->EndRenderPass();

        command_list->ResourceBarrier({ { back_buffer, ResourceState::kRenderTarget, ResourceState::kPresent } });
        command_list->Close();
    }
}

void DepthStencilReadRenderer::Resize(const NativeSurface& surface, uint32_t width, uint32_t height)
{
    WaitForIdle();
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        back_buffer_views_[i].reset();
    }
    swapchain_.reset();
    Init(surface, width, height);
}

void DepthStencilReadRenderer::Render()
{
    uint32_t frame_index = swapchain_->NextImage(fence_, ++fence_value_);
    command_queue_->Wait(fence_, fence_value_);
    fence_->Wait(fence_values_[frame_index]);
    command_queue_->ExecuteCommandLists({ command_lists_[frame_index] });
    command_queue_->Signal(fence_, fence_values_[frame_index] = ++fence_value_);
    swapchain_->Present(fence_, fence_values_[frame_index]);
}

std::string_view DepthStencilReadRenderer::GetTitle() const
{
    return "DepthStencilRead";
}

const std::string& DepthStencilReadRenderer::GetGpuName() const
{
    return adapter_->GetName();
}

ApiType DepthStencilReadRenderer::GetApiType() const
{
    return settings_.api_type;
}

void DepthStencilReadRenderer::WaitForIdle()
{
    command_queue_->Signal(fence_, ++fence_value_);
    fence_->Wait(fence_value_);
}

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppLoop::Run(std::make_unique<DepthStencilReadRenderer>(settings), argc, argv);
}

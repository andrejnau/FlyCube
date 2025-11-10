#include "ApiType/ApiType.h"
#include "AppLoop/AppLoop.h"
#include "AppSettings/ArgsParser.h"
#include "Instance/Instance.h"
#include "Utilities/Asset.h"

namespace {

constexpr uint32_t kFrameCount = 3;

} // namespace

class TriangleRenderer : public AppRenderer {
public:
    TriangleRenderer(const Settings& settings);
    ~TriangleRenderer() override;

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
    std::shared_ptr<Resource> m_index_buffer;
    std::shared_ptr<Resource> m_vertex_buffer;
    std::shared_ptr<Resource> m_constant_buffer;
    std::shared_ptr<View> m_constant_buffer_view;
    std::shared_ptr<Shader> m_vertex_shader;
    std::shared_ptr<Shader> m_pixel_shader;
    std::shared_ptr<BindingSetLayout> m_layout;
    std::shared_ptr<BindingSet> m_binding_set;

    std::shared_ptr<Swapchain> m_swapchain;
    std::shared_ptr<Pipeline> m_pipeline;
    std::array<std::shared_ptr<View>, kFrameCount> m_back_buffer_views = {};
    std::array<std::shared_ptr<CommandList>, kFrameCount> m_command_lists = {};
    std::array<uint64_t, kFrameCount> m_fence_values = {};
};

TriangleRenderer::TriangleRenderer(const Settings& settings)
    : m_settings(settings)
{
    m_instance = CreateInstance(m_settings.api_type);
    m_adapter = std::move(m_instance->EnumerateAdapters()[m_settings.required_gpu_index]);
    m_device = m_adapter->CreateDevice();
    m_command_queue = m_device->GetCommandQueue(CommandListType::kGraphics);
    m_fence = m_device->CreateFence(m_fence_value);

    std::vector<uint32_t> index_data = { 0, 1, 2 };
    m_index_buffer = m_device->CreateBuffer(
        MemoryType::kUpload,
        { .size = sizeof(index_data.front()) * index_data.size(), .usage = BindFlag::kIndexBuffer });
    m_index_buffer->UpdateUploadBuffer(0, index_data.data(), sizeof(index_data.front()) * index_data.size());

    std::vector<glm::vec3> vertex_data = {
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3(0.0, 0.5, 0.0),
        glm::vec3(0.5, -0.5, 0.0),
    };
    m_vertex_buffer = m_device->CreateBuffer(
        MemoryType::kUpload,
        { .size = sizeof(vertex_data.front()) * vertex_data.size(), .usage = BindFlag::kVertexBuffer });
    m_vertex_buffer->UpdateUploadBuffer(0, vertex_data.data(), sizeof(vertex_data.front()) * vertex_data.size());

    glm::vec4 constant_data = glm::vec4(1.0, 0.0, 0.0, 1.0);
    m_constant_buffer = m_device->CreateBuffer(MemoryType::kUpload,
                                               { .size = sizeof(constant_data), .usage = BindFlag::kConstantBuffer });
    m_constant_buffer->UpdateUploadBuffer(0, &constant_data, sizeof(constant_data));
    ViewDesc constant_buffer_view_desc = {
        .view_type = ViewType::kConstantBuffer,
        .dimension = ViewDimension::kBuffer,
    };
    m_constant_buffer_view = m_device->CreateView(m_constant_buffer, constant_buffer_view_desc);

    ShaderBlobType blob_type = m_device->GetSupportedShaderBlobType();
    std::vector<uint8_t> vertex_blob = AssetLoadShaderBlob("assets/Triangle/VertexShader.hlsl", blob_type);
    std::vector<uint8_t> pixel_blob = AssetLoadShaderBlob("assets/Triangle/PixelShader.hlsl", blob_type);
    m_vertex_shader = m_device->CreateShader(vertex_blob, blob_type, ShaderType::kVertex);
    m_pixel_shader = m_device->CreateShader(pixel_blob, blob_type, ShaderType::kPixel);

    BindKey constant_buffer_key = {
        .shader_type = ShaderType::kPixel,
        .view_type = ViewType::kConstantBuffer,
        .slot = 0,
        .space = 0,
        .count = 1,
    };
    m_layout = m_device->CreateBindingSetLayout({ constant_buffer_key });
    m_binding_set = m_device->CreateBindingSet(m_layout);
    m_binding_set->WriteBindings({ { constant_buffer_key, m_constant_buffer_view } });
}

TriangleRenderer::~TriangleRenderer()
{
    WaitForIdle();
}

void TriangleRenderer::Init(const AppSize& app_size, WindowHandle window)
{
    m_swapchain = m_device->CreateSwapchain(window, app_size.width(), app_size.height(), kFrameCount, m_settings.vsync);

    GraphicsPipelineDesc pipeline_desc = {
        .program = m_device->CreateProgram({ m_vertex_shader, m_pixel_shader }),
        .layout = m_layout,
        .input = { { 0, "POSITION", gli::FORMAT_RGB32_SFLOAT_PACK32, sizeof(glm::vec3) } },
        .color_formats = { m_swapchain->GetFormat() },
    };
    m_pipeline = m_device->CreateGraphicsPipeline(pipeline_desc);

    for (uint32_t i = 0; i < kFrameCount; ++i) {
        std::shared_ptr<Resource> back_buffer = m_swapchain->GetBackBuffer(i);
        ViewDesc back_buffer_view_desc = {
            .view_type = ViewType::kRenderTarget,
            .dimension = ViewDimension::kTexture2D,
        };
        m_back_buffer_views[i] = m_device->CreateView(back_buffer, back_buffer_view_desc);

        auto& command_list = m_command_lists[i];
        command_list = m_device->CreateCommandList(CommandListType::kGraphics);
        command_list->BindPipeline(m_pipeline);
        command_list->BindBindingSet(m_binding_set);
        command_list->SetViewport(0, 0, app_size.width(), app_size.height());
        command_list->SetScissorRect(0, 0, app_size.width(), app_size.height());
        command_list->IASetIndexBuffer(m_index_buffer, 0, gli::format::FORMAT_R32_UINT_PACK32);
        command_list->IASetVertexBuffer(0, m_vertex_buffer, 0);
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kRenderTarget } });
        RenderPassDesc render_pass_desc = {
            .colors = { { .load_op = RenderPassLoadOp::kClear,
                          .store_op = RenderPassStoreOp::kStore,
                          .clear_value = glm::vec4(0.0, 0.2, 0.4, 1.0) } },
        };
        FramebufferDesc framebuffer_desc = {
            .width = app_size.width(),
            .height = app_size.height(),
            .colors = { m_back_buffer_views[i] },
        };
        command_list->BeginRenderPass(render_pass_desc, framebuffer_desc);
        command_list->DrawIndexed(3, 1, 0, 0, 0);
        command_list->EndRenderPass();
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kRenderTarget, ResourceState::kPresent } });
        command_list->Close();
    }
}

void TriangleRenderer::Resize(const AppSize& app_size, WindowHandle window)
{
    WaitForIdle();
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        m_back_buffer_views[i].reset();
    }
    m_swapchain.reset();
    Init(app_size, window);
}

void TriangleRenderer::Render()
{
    uint32_t frame_index = m_swapchain->NextImage(m_fence, ++m_fence_value);
    m_command_queue->Wait(m_fence, m_fence_value);
    m_fence->Wait(m_fence_values[frame_index]);
    m_command_queue->ExecuteCommandLists({ m_command_lists[frame_index] });
    m_command_queue->Signal(m_fence, m_fence_values[frame_index] = ++m_fence_value);
    m_swapchain->Present(m_fence, m_fence_values[frame_index]);
}

std::string_view TriangleRenderer::GetTitle() const
{
    return "Triangle";
}

const std::string& TriangleRenderer::GetGpuName() const
{
    return m_adapter->GetName();
}

const Settings& TriangleRenderer::GetSettings() const
{
    return m_settings;
}

void TriangleRenderer::WaitForIdle()
{
    m_command_queue->Signal(m_fence, ++m_fence_value);
    m_fence->Wait(m_fence_value);
}

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppLoop::Run(std::make_unique<TriangleRenderer>(settings), argc, argv);
}

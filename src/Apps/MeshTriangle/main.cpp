#include "ApiType/ApiType.h"
#include "AppLoop/AppLoop.h"
#include "AppSettings/ArgsParser.h"
#include "Instance/Instance.h"
#include "Utilities/Asset.h"

#include <stdexcept>

class MeshTriangleRenderer : public AppRenderer {
public:
    MeshTriangleRenderer(const Settings& settings);
    ~MeshTriangleRenderer() override;

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
    std::shared_ptr<Swapchain> m_swapchain;
    uint64_t m_fence_value = 0;
    std::shared_ptr<Fence> m_fence;
    static constexpr uint32_t kFrameCount = 3;
    std::array<uint64_t, kFrameCount> m_fence_values = {};
    std::vector<std::shared_ptr<Framebuffer>> m_framebuffers;
    std::array<std::shared_ptr<CommandList>, kFrameCount> m_command_lists;
    std::shared_ptr<Shader> m_mesh_shader;
    std::shared_ptr<Shader> m_pixel_shader;
    std::shared_ptr<Program> m_program;
    std::shared_ptr<BindingSetLayout> m_layout;
    std::shared_ptr<RenderPass> m_render_pass;
    std::shared_ptr<Pipeline> m_pipeline;
};

MeshTriangleRenderer::MeshTriangleRenderer(const Settings& settings)
    : m_settings(settings)
{
    m_instance = CreateInstance(m_settings.api_type);
    m_adapter = std::move(m_instance->EnumerateAdapters()[m_settings.required_gpu_index]);
    m_device = m_adapter->CreateDevice();
    if (!m_device->IsMeshShadingSupported()) {
        throw std::runtime_error("Mesh Shading is not supported");
    }
    m_command_queue = m_device->GetCommandQueue(CommandListType::kGraphics);
    m_fence = m_device->CreateFence(m_fence_value);
}

MeshTriangleRenderer::~MeshTriangleRenderer()
{
    WaitForIdle();
}

void MeshTriangleRenderer::Init(const AppSize& app_size, WindowHandle window)
{
    m_swapchain = m_device->CreateSwapchain(window, app_size.width(), app_size.height(), kFrameCount, m_settings.vsync);

    ShaderBlobType blob_type = m_device->GetSupportedShaderBlobType();
    std::vector<uint8_t> mesh_blob = AssetLoadShaderBlob("assets/MeshTriangle/MeshShader.hlsl", blob_type);
    std::vector<uint8_t> pixel_blob = AssetLoadShaderBlob("assets/MeshTriangle/PixelShader.hlsl", blob_type);
    m_mesh_shader = m_device->CreateShader(mesh_blob, blob_type, ShaderType::kMesh);
    m_pixel_shader = m_device->CreateShader(pixel_blob, blob_type, ShaderType::kPixel);
    m_program = m_device->CreateProgram({ m_mesh_shader, m_pixel_shader });
    m_layout = m_device->CreateBindingSetLayout({});

    RenderPassDesc render_pass_desc = {
        { { m_swapchain->GetFormat(), RenderPassLoadOp::kClear, RenderPassStoreOp::kStore } },
    };
    m_render_pass = m_device->CreateRenderPass(render_pass_desc);
    ClearDesc clear_desc = { { { 0.0, 0.2, 0.4, 1.0 } } };
    GraphicsPipelineDesc pipeline_desc = { m_program, m_layout, {}, m_render_pass };
    m_pipeline = m_device->CreateGraphicsPipeline(pipeline_desc);

    for (uint32_t i = 0; i < kFrameCount; ++i) {
        ViewDesc back_buffer_view_desc = {
            .view_type = ViewType::kRenderTarget,
            .dimension = ViewDimension::kTexture2D,
        };
        std::shared_ptr<Resource> back_buffer = m_swapchain->GetBackBuffer(i);
        std::shared_ptr<View> back_buffer_view = m_device->CreateView(back_buffer, back_buffer_view_desc);
        FramebufferDesc framebuffer_desc = {
            .render_pass = m_render_pass,
            .width = app_size.width(),
            .height = app_size.height(),
            .colors = { back_buffer_view },
        };
        std::shared_ptr<Framebuffer> framebuffer =
            m_framebuffers.emplace_back(m_device->CreateFramebuffer(framebuffer_desc));
        decltype(auto) command_list = m_command_lists[i];
        command_list = m_device->CreateCommandList(CommandListType::kGraphics);
        command_list->BindPipeline(m_pipeline);
        command_list->SetViewport(0, 0, app_size.width(), app_size.height());
        command_list->SetScissorRect(0, 0, app_size.width(), app_size.height());
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kRenderTarget } });
        command_list->BeginRenderPass(m_render_pass, framebuffer, clear_desc);
        command_list->DispatchMesh(1, 1, 1);
        command_list->EndRenderPass();
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kRenderTarget, ResourceState::kPresent } });
        command_list->Close();
    }
}

void MeshTriangleRenderer::Resize(const AppSize& app_size, WindowHandle window)
{
    WaitForIdle();
    Init(app_size, window);
}

void MeshTriangleRenderer::Render()
{
    uint32_t frame_index = m_swapchain->NextImage(m_fence, ++m_fence_value);
    m_command_queue->Wait(m_fence, m_fence_value);
    m_fence->Wait(m_fence_values[frame_index]);
    m_command_queue->ExecuteCommandLists({ m_command_lists[frame_index] });
    m_command_queue->Signal(m_fence, m_fence_values[frame_index] = ++m_fence_value);
    m_swapchain->Present(m_fence, m_fence_values[frame_index]);
}

std::string_view MeshTriangleRenderer::GetTitle() const
{
    return "MeshTriangle";
}

const std::string& MeshTriangleRenderer::GetGpuName() const
{
    return m_adapter->GetName();
}

const Settings& MeshTriangleRenderer::GetSettings() const
{
    return m_settings;
}

void MeshTriangleRenderer::WaitForIdle()
{
    m_command_queue->Signal(m_fence, ++m_fence_value);
    m_fence->Wait(m_fence_value);
}

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppLoop::Run(std::make_unique<MeshTriangleRenderer>(settings), argc, argv);
}

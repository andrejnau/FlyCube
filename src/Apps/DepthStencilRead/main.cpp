#include "ApiType/ApiType.h"
#include "AppLoop/AppLoop.h"
#include "AppSettings/ArgsParser.h"
#include "Instance/Instance.h"
#include "RenderUtils/ModelLoader.h"
#include "RenderUtils/RenderModel.h"
#include "Utilities/Asset.h"

namespace {

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

constexpr uint32_t kPositions = 0;
constexpr uint32_t kTexcoords = 1;
constexpr uint32_t kFrameCount = 3;

} // namespace

class DepthStencilReadRenderer : public AppRenderer {
public:
    DepthStencilReadRenderer(const Settings& settings);
    ~DepthStencilReadRenderer() override;

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
    RenderModel m_render_model;
    RenderModel m_fullscreen_triangle_render_model;
    std::shared_ptr<Resource> m_depth_stencil_pass_constant_buffer;
    std::vector<std::shared_ptr<View>> m_depth_stencil_pass_constant_buffer_views;
    std::shared_ptr<Resource> m_vertex_constant_buffer;
    std::shared_ptr<View> m_vertex_constant_buffer_view;
    std::shared_ptr<Resource> m_pixel_constant_buffer;
    std::shared_ptr<View> m_pixel_constant_buffer_view;
    std::shared_ptr<Shader> m_vertex_shader;
    std::shared_ptr<Shader> m_pixel_shader;
    std::shared_ptr<BindingSetLayout> m_layout;
    std::vector<std::shared_ptr<BindingSet>> m_depth_stencil_pass_binding_sets;

    std::shared_ptr<Swapchain> m_swapchain;
    std::shared_ptr<Resource> m_depth_stencil_texture;
    std::shared_ptr<View> m_depth_stencil_view;
    std::shared_ptr<View> m_depth_read_view;
    std::shared_ptr<View> m_stencil_read_view;
    std::shared_ptr<Pipeline> m_depth_stencil_pass_pipeline;
    std::shared_ptr<Pipeline> m_pipeline;
    std::shared_ptr<BindingSet> m_binding_set;
    std::array<std::shared_ptr<View>, kFrameCount> m_back_buffer_views = {};
    std::array<std::shared_ptr<CommandList>, kFrameCount> m_command_lists = {};
    std::array<uint64_t, kFrameCount> m_fence_values = {};
};

DepthStencilReadRenderer::DepthStencilReadRenderer(const Settings& settings)
    : m_settings(settings)
{
    m_instance = CreateInstance(m_settings.api_type);
    m_adapter = std::move(m_instance->EnumerateAdapters()[m_settings.required_gpu_index]);
    m_device = m_adapter->CreateDevice();
    m_command_queue = m_device->GetCommandQueue(CommandListType::kGraphics);
    m_fence = m_device->CreateFence(m_fence_value);

    std::unique_ptr<Model> model = LoadModel("assets/DepthStencilRead/DamagedHelmet.gltf");
    m_render_model = RenderModel(m_device, m_command_queue, std::move(model));

    Model fullscreen_triangle_model = {
        .meshes = { {
            .indices = { 0, 1, 2 },
            .positions = { glm::vec3(-1.0, 1.0, 0.0), glm::vec3(3.0, 1.0, 0.0), glm::vec3(-1.0, -3.0, 0.0) },
            .texcoords = { glm::vec2(0.0, 0.0), glm::vec2(2.0, 0.0), glm::vec2(0.0, 2.0) },
        } },
    };
    m_fullscreen_triangle_render_model = RenderModel(m_device, m_command_queue, &fullscreen_triangle_model);

    m_depth_stencil_pass_constant_buffer = m_device->CreateBuffer(
        MemoryType::kUpload,
        { .size = sizeof(glm::mat4) * m_render_model.GetMeshCount(), .usage = BindFlag::kConstantBuffer });
    m_depth_stencil_pass_constant_buffer_views.resize(m_render_model.GetMeshCount());
    for (size_t i = 0; i < m_render_model.GetMeshCount(); ++i) {
        ViewDesc depth_stencil_pass_constant_buffer_view_desc = {
            .view_type = ViewType::kConstantBuffer,
            .dimension = ViewDimension::kBuffer,
            .offset = i * sizeof(glm::mat4),
        };
        m_depth_stencil_pass_constant_buffer_views[i] =
            m_device->CreateView(m_depth_stencil_pass_constant_buffer, depth_stencil_pass_constant_buffer_view_desc);
    }

    glm::mat4 vertex_constant_data = glm::transpose(glm::mat4(1.0));
    m_vertex_constant_buffer = m_device->CreateBuffer(
        MemoryType::kUpload, { .size = sizeof(vertex_constant_data), .usage = BindFlag::kConstantBuffer });
    ViewDesc vertex_constant_buffer_view_desc = {
        .view_type = ViewType::kConstantBuffer,
        .dimension = ViewDimension::kBuffer,
    };
    m_vertex_constant_buffer_view = m_device->CreateView(m_vertex_constant_buffer, vertex_constant_buffer_view_desc);
    m_vertex_constant_buffer->UpdateUploadBuffer(0, &vertex_constant_data, sizeof(vertex_constant_data));

    m_pixel_constant_buffer =
        m_device->CreateBuffer(MemoryType::kUpload, { .size = sizeof(glm::uvec2), .usage = BindFlag::kConstantBuffer });
    ViewDesc pixel_constant_buffer_view_desc = {
        .view_type = ViewType::kConstantBuffer,
        .dimension = ViewDimension::kBuffer,
    };
    m_pixel_constant_buffer_view = m_device->CreateView(m_pixel_constant_buffer, pixel_constant_buffer_view_desc);

    ShaderBlobType blob_type = m_device->GetSupportedShaderBlobType();
    std::vector<uint8_t> vertex_blob = AssetLoadShaderBlob("assets/DepthStencilRead/VertexShader.hlsl", blob_type);
    std::vector<uint8_t> pixel_blob = AssetLoadShaderBlob("assets/DepthStencilRead/PixelShader.hlsl", blob_type);
    m_vertex_shader = m_device->CreateShader(vertex_blob, blob_type, ShaderType::kVertex);
    m_pixel_shader = m_device->CreateShader(pixel_blob, blob_type, ShaderType::kPixel);

    BindKey vertex_constant_buffer_key = m_vertex_shader->GetBindKey("constant_buffer");
    BindKey pixel_constant_buffer_key = m_pixel_shader->GetBindKey("constant_buffer");
    BindKey pixel_depth_buffer_key = m_pixel_shader->GetBindKey("depth_buffer");
    BindKey pixel_stencil_buffer_key = m_pixel_shader->GetBindKey("stencil_buffer");
    m_layout = m_device->CreateBindingSetLayout(
        { vertex_constant_buffer_key, pixel_constant_buffer_key, pixel_depth_buffer_key, pixel_stencil_buffer_key });

    m_depth_stencil_pass_binding_sets.resize(m_render_model.GetMeshCount());
    for (size_t i = 0; i < m_render_model.GetMeshCount(); ++i) {
        m_depth_stencil_pass_binding_sets[i] = m_device->CreateBindingSet(m_layout);
        m_depth_stencil_pass_binding_sets[i]->WriteBindings({
            { vertex_constant_buffer_key, m_depth_stencil_pass_constant_buffer_views[i] },
        });
    }
}

DepthStencilReadRenderer::~DepthStencilReadRenderer()
{
    WaitForIdle();
}

void DepthStencilReadRenderer::Init(const AppSize& app_size, WindowHandle window)
{
    m_swapchain = m_device->CreateSwapchain(window, app_size.width(), app_size.height(), kFrameCount, m_settings.vsync);

    glm::mat4 view = GetViewMatrix();
    glm::uvec2 depth_stencil_size = glm::uvec2(app_size.width() / 2, app_size.height());
    glm::mat4 projection = GetProjectionMatrix(depth_stencil_size.x, depth_stencil_size.y);

    std::vector<glm::mat4> depth_stencil_pass_constant_data(m_render_model.GetMeshCount());
    for (size_t i = 0; i < m_render_model.GetMeshCount(); ++i) {
        depth_stencil_pass_constant_data[i] = glm::transpose(projection * view * m_render_model.GetMesh(i).matrix);
    }
    m_depth_stencil_pass_constant_buffer->UpdateUploadBuffer(
        0, depth_stencil_pass_constant_data.data(),
        sizeof(depth_stencil_pass_constant_data.front()) * depth_stencil_pass_constant_data.size());

    glm::uvec2 pixel_constant_data = glm::uvec2(app_size.width(), app_size.height());
    m_pixel_constant_buffer->UpdateUploadBuffer(0, &pixel_constant_data, sizeof(pixel_constant_data));

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
    m_depth_stencil_texture = m_device->CreateTexture(MemoryType::kDefault, depth_stencil_texture_desc);
    ViewDesc depth_stencil_view_desc = {
        .view_type = ViewType::kDepthStencil,
        .dimension = ViewDimension::kTexture2D,
    };
    m_depth_stencil_view = m_device->CreateView(m_depth_stencil_texture, depth_stencil_view_desc);
    ViewDesc depth_read_view_desc = {
        .view_type = ViewType::kTexture,
        .dimension = ViewDimension::kTexture2D,
        .plane_slice = 0,
    };
    m_depth_read_view = m_device->CreateView(m_depth_stencil_texture, depth_read_view_desc);
    ViewDesc stencil_read_view_desc = {
        .view_type = ViewType::kTexture,
        .dimension = ViewDimension::kTexture2D,
        .plane_slice = 1,
    };
    m_stencil_read_view = m_device->CreateView(m_depth_stencil_texture, stencil_read_view_desc);

    RenderPassDesc depth_stencil_pass_render_pass_desc = {
        .depth_stencil = { m_depth_stencil_texture->GetFormat(), RenderPassLoadOp::kClear, RenderPassStoreOp::kStore,
                           RenderPassLoadOp::kClear, RenderPassStoreOp::kStore },
    };

    DepthStencilDesc depth_stencil_pass_depth_stencil_desc = {
        .stencil_enable = true,
        .front_face = { .depth_fail_op = StencilOp::kIncr, .pass_op = StencilOp::kIncr },
        .back_face = { .depth_fail_op = StencilOp::kIncr, .pass_op = StencilOp::kIncr },
    };

    GraphicsPipelineDesc depth_stencil_pass_pipeline_desc = {
        .program = m_device->CreateProgram({ m_vertex_shader }),
        .layout = m_layout,
        .input = { { kPositions, "POSITION", gli::FORMAT_RGB32_SFLOAT_PACK32, sizeof(glm::vec3) },
                   { kTexcoords, "TEXCOORD", gli::FORMAT_RG32_SFLOAT_PACK32, sizeof(glm::vec2) } },
        .depth_stencil_format = m_depth_stencil_texture->GetFormat(),
        .depth_stencil_desc = depth_stencil_pass_depth_stencil_desc,
    };
    m_depth_stencil_pass_pipeline = m_device->CreateGraphicsPipeline(depth_stencil_pass_pipeline_desc);

    RenderPassDesc render_pass_desc = {
        .colors = { { m_swapchain->GetFormat(), RenderPassLoadOp::kClear, RenderPassStoreOp::kStore } },
    };

    DepthStencilDesc depth_stencil_desc = {
        .depth_test_enable = false,
        .depth_func = ComparisonFunc::kAlways,
        .depth_write_enable = false,
    };

    GraphicsPipelineDesc pipeline_desc = {
        .program = m_device->CreateProgram({ m_vertex_shader, m_pixel_shader }),
        .layout = m_layout,
        .input = { { kPositions, "POSITION", gli::FORMAT_RGB32_SFLOAT_PACK32, sizeof(glm::vec3) },
                   { kTexcoords, "TEXCOORD", gli::FORMAT_RG32_SFLOAT_PACK32, sizeof(glm::vec2) } },
        .color_formats = { m_swapchain->GetFormat() },
        .depth_stencil_desc = depth_stencil_desc,
    };
    m_pipeline = m_device->CreateGraphicsPipeline(pipeline_desc);

    BindKey vertex_constant_buffer_key = m_vertex_shader->GetBindKey("constant_buffer");
    BindKey pixel_constant_buffer_key = m_pixel_shader->GetBindKey("constant_buffer");
    BindKey pixel_depth_buffer_key = m_pixel_shader->GetBindKey("depth_buffer");
    BindKey pixel_stencil_buffer_key = m_pixel_shader->GetBindKey("stencil_buffer");

    m_binding_set = m_device->CreateBindingSet(m_layout);
    m_binding_set->WriteBindings({
        { vertex_constant_buffer_key, m_vertex_constant_buffer_view },
        { pixel_constant_buffer_key, m_pixel_constant_buffer_view },
        { pixel_depth_buffer_key, m_depth_read_view },
        { pixel_stencil_buffer_key, m_stencil_read_view },
    });

    for (uint32_t i = 0; i < kFrameCount; ++i) {
        std::shared_ptr<Resource> back_buffer = m_swapchain->GetBackBuffer(i);
        ViewDesc back_buffer_view_desc = {
            .view_type = ViewType::kRenderTarget,
            .dimension = ViewDimension::kTexture2D,
        };
        m_back_buffer_views[i] = m_device->CreateView(back_buffer, back_buffer_view_desc);

        FramebufferDesc depth_stencil_pass_framebuffer_desc = {
            .width = depth_stencil_size.x,
            .height = depth_stencil_size.y,
            .depth_stencil = m_depth_stencil_view,
        };
        FramebufferDesc framebuffer_desc = {
            .width = app_size.width(),
            .height = app_size.height(),
            .colors = { m_back_buffer_views[i] },
        };

        auto& command_list = m_command_lists[i];
        command_list = m_device->CreateCommandList(CommandListType::kGraphics);
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kRenderTarget } });

        command_list->BindPipeline(m_depth_stencil_pass_pipeline);
        command_list->BeginRenderPass(depth_stencil_pass_render_pass_desc, depth_stencil_pass_framebuffer_desc,
                                      /*clear_desc=*/{});
        command_list->SetViewport(0, 0, depth_stencil_size.x, depth_stencil_size.y);
        command_list->SetScissorRect(0, 0, depth_stencil_size.x, depth_stencil_size.y);
        for (size_t j = 0; j < m_render_model.GetMeshCount(); ++j) {
            const auto& mesh = m_render_model.GetMesh(j);
            command_list->BindBindingSet(m_depth_stencil_pass_binding_sets[j]);
            command_list->IASetIndexBuffer(mesh.indices, 0, mesh.index_format);
            command_list->IASetVertexBuffer(kPositions, mesh.positions, 0);
            command_list->IASetVertexBuffer(kTexcoords, mesh.texcoords, 0);
            command_list->DrawIndexed(mesh.index_count, 1, 0, 0, 0);
        }
        command_list->EndRenderPass();

        command_list->BindPipeline(m_pipeline);
        command_list->BeginRenderPass(render_pass_desc, framebuffer_desc, /*clear_desc=*/{});
        command_list->SetViewport(0, 0, app_size.width(), app_size.height());
        command_list->SetScissorRect(0, 0, app_size.width(), app_size.height());
        for (size_t j = 0; j < m_fullscreen_triangle_render_model.GetMeshCount(); ++j) {
            const auto& mesh = m_fullscreen_triangle_render_model.GetMesh(j);
            command_list->BindBindingSet(m_binding_set);
            command_list->IASetIndexBuffer(mesh.indices, 0, mesh.index_format);
            command_list->IASetVertexBuffer(kPositions, mesh.positions, 0);
            command_list->IASetVertexBuffer(kTexcoords, mesh.texcoords, 0);
            command_list->DrawIndexed(mesh.index_count, 1, 0, 0, 0);
        }
        command_list->EndRenderPass();

        command_list->ResourceBarrier({ { back_buffer, ResourceState::kRenderTarget, ResourceState::kPresent } });
        command_list->Close();
    }
}

void DepthStencilReadRenderer::Resize(const AppSize& app_size, WindowHandle window)
{
    WaitForIdle();
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        m_back_buffer_views[i].reset();
    }
    m_swapchain.reset();
    Init(app_size, window);
}

void DepthStencilReadRenderer::Render()
{
    uint32_t frame_index = m_swapchain->NextImage(m_fence, ++m_fence_value);
    m_command_queue->Wait(m_fence, m_fence_value);
    m_fence->Wait(m_fence_values[frame_index]);
    m_command_queue->ExecuteCommandLists({ m_command_lists[frame_index] });
    m_command_queue->Signal(m_fence, m_fence_values[frame_index] = ++m_fence_value);
    m_swapchain->Present(m_fence, m_fence_values[frame_index]);
}

std::string_view DepthStencilReadRenderer::GetTitle() const
{
    return "DepthStencilRead";
}

const std::string& DepthStencilReadRenderer::GetGpuName() const
{
    return m_adapter->GetName();
}

const Settings& DepthStencilReadRenderer::GetSettings() const
{
    return m_settings;
}

void DepthStencilReadRenderer::WaitForIdle()
{
    m_command_queue->Signal(m_fence, ++m_fence_value);
    m_fence->Wait(m_fence_value);
}

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppLoop::Run(std::make_unique<DepthStencilReadRenderer>(settings), argc, argv);
}

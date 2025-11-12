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

class ModelViewRenderer : public AppRenderer {
public:
    ModelViewRenderer(const Settings& settings);
    ~ModelViewRenderer() override;

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
    std::shared_ptr<Resource> m_vertex_constant_buffer;
    std::vector<std::shared_ptr<View>> m_vertex_constant_buffer_views;
    std::vector<std::shared_ptr<View>> m_pixel_textures_views;
    std::shared_ptr<Resource> m_pixel_anisotropic_sampler;
    std::shared_ptr<View> m_pixel_anisotropic_sampler_view;
    std::shared_ptr<Resource> m_pixel_constant_buffer;
    std::vector<std::shared_ptr<View>> m_pixel_constant_buffer_views;
    std::shared_ptr<Shader> m_vertex_shader;
    std::shared_ptr<Shader> m_pixel_shader;
    std::shared_ptr<BindingSetLayout> m_layout;
    std::vector<std::shared_ptr<BindingSet>> m_binding_sets;

    std::shared_ptr<Swapchain> m_swapchain;
    std::shared_ptr<Resource> m_depth_stencil_texture;
    std::shared_ptr<View> m_depth_stencil_view;
    std::shared_ptr<Pipeline> m_pipeline;
    std::array<std::shared_ptr<View>, kFrameCount> m_back_buffer_views = {};
    std::array<std::shared_ptr<CommandList>, kFrameCount> m_command_lists = {};
    std::array<uint64_t, kFrameCount> m_fence_values = {};
};

ModelViewRenderer::ModelViewRenderer(const Settings& settings)
    : m_settings(settings)
{
    m_instance = CreateInstance(m_settings.api_type);
    m_adapter = std::move(m_instance->EnumerateAdapters()[m_settings.required_gpu_index]);
    m_device = m_adapter->CreateDevice();
    m_command_queue = m_device->GetCommandQueue(CommandListType::kGraphics);
    m_fence = m_device->CreateFence(m_fence_value);

    std::unique_ptr<Model> model = LoadModel("assets/ModelView/DamagedHelmet.gltf");
    m_render_model = RenderModel(m_device, m_command_queue, std::move(model));

    m_vertex_constant_buffer = m_device->CreateBuffer(
        MemoryType::kUpload,
        { .size = sizeof(glm::mat4) * m_render_model.GetMeshCount(), .usage = BindFlag::kConstantBuffer });
    m_vertex_constant_buffer_views.resize(m_render_model.GetMeshCount());
    for (size_t i = 0; i < m_render_model.GetMeshCount(); ++i) {
        ViewDesc vertex_constant_buffer_view_desc = {
            .view_type = ViewType::kConstantBuffer,
            .dimension = ViewDimension::kBuffer,
            .offset = i * sizeof(glm::mat4),
        };
        m_vertex_constant_buffer_views[i] =
            m_device->CreateView(m_vertex_constant_buffer, vertex_constant_buffer_view_desc);
    }

    m_pixel_textures_views.resize(m_render_model.GetMeshCount());
    for (size_t i = 0; i < m_render_model.GetMeshCount(); ++i) {
        ViewDesc pixel_textures_view_desc = {
            .view_type = ViewType::kTexture,
            .dimension = ViewDimension::kTexture2D,
            .bindless = m_device->IsBindlessSupported(),
        };
        m_pixel_textures_views[i] =
            m_device->CreateView(m_render_model.GetMesh(i).textures.base_color, pixel_textures_view_desc);
    }

    m_pixel_anisotropic_sampler = m_device->CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever,
    });
    ViewDesc pixel_anisotropic_sampler_view_desc = {
        .view_type = ViewType::kSampler,
    };
    m_pixel_anisotropic_sampler_view =
        m_device->CreateView(m_pixel_anisotropic_sampler, pixel_anisotropic_sampler_view_desc);

    if (m_device->IsBindlessSupported()) {
        std::vector<uint32_t> pixel_constant_data(m_render_model.GetMeshCount());
        m_pixel_constant_buffer = m_device->CreateBuffer(
            MemoryType::kUpload, { .size = sizeof(pixel_constant_data.front()) * pixel_constant_data.size(),
                                   .usage = BindFlag::kConstantBuffer });
        m_pixel_constant_buffer_views.resize(m_render_model.GetMeshCount());
        for (size_t i = 0; i < m_render_model.GetMeshCount(); ++i) {
            ViewDesc pixel_constant_buffer_view_desc = {
                .view_type = ViewType::kConstantBuffer,
                .dimension = ViewDimension::kBuffer,
                .offset = i * sizeof(pixel_constant_data.front()),
            };
            m_pixel_constant_buffer_views[i] =
                m_device->CreateView(m_pixel_constant_buffer, pixel_constant_buffer_view_desc);
            pixel_constant_data[i] = m_pixel_textures_views[i]->GetDescriptorId();
        }
        m_pixel_constant_buffer->UpdateUploadBuffer(0, pixel_constant_data.data(),
                                                    sizeof(pixel_constant_data.front()) * pixel_constant_data.size());
    }

    ShaderBlobType blob_type = m_device->GetSupportedShaderBlobType();
    std::vector<uint8_t> vertex_blob = AssetLoadShaderBlob("assets/ModelView/VertexShader.hlsl", blob_type);
    std::vector<uint8_t> pixel_blob =
        AssetLoadShaderBlob(m_device->IsBindlessSupported() ? "assets/ModelView/PixelShaderBindless.hlsl"
                                                            : "assets/ModelView/PixelShader.hlsl",
                            blob_type);
    m_vertex_shader = m_device->CreateShader(vertex_blob, blob_type, ShaderType::kVertex);
    m_pixel_shader = m_device->CreateShader(pixel_blob, blob_type, ShaderType::kPixel);

    BindKey vertex_constant_buffer_key = m_vertex_shader->GetBindKey("constant_buffer");
    BindKey pixel_anisotropic_sampler_key = m_pixel_shader->GetBindKey("anisotropic_sampler");
    BindKey pixel_bindless_textures_key;
    BindKey pixel_constant_buffer_key;
    BindKey pixel_base_color_texture_key;

    if (m_device->IsBindlessSupported()) {
        pixel_bindless_textures_key = m_pixel_shader->GetBindKey("bindless_textures");
        pixel_constant_buffer_key = m_pixel_shader->GetBindKey("constant_buffer");
        m_layout = m_device->CreateBindingSetLayout({ vertex_constant_buffer_key, pixel_bindless_textures_key,
                                                      pixel_anisotropic_sampler_key, pixel_constant_buffer_key });
    } else {
        pixel_base_color_texture_key = m_pixel_shader->GetBindKey("base_color_texture");
        m_layout = m_device->CreateBindingSetLayout(
            { vertex_constant_buffer_key, pixel_base_color_texture_key, pixel_anisotropic_sampler_key });
    }

    m_binding_sets.resize(m_render_model.GetMeshCount());
    for (size_t i = 0; i < m_render_model.GetMeshCount(); ++i) {
        m_binding_sets[i] = m_device->CreateBindingSet(m_layout);
        if (m_device->IsBindlessSupported()) {
            m_binding_sets[i]->WriteBindings({
                { vertex_constant_buffer_key, m_vertex_constant_buffer_views[i] },
                { pixel_anisotropic_sampler_key, m_pixel_anisotropic_sampler_view },
                { pixel_constant_buffer_key, m_pixel_constant_buffer_views[i] },
            });
        } else {
            m_binding_sets[i]->WriteBindings({
                { vertex_constant_buffer_key, m_vertex_constant_buffer_views[i] },
                { pixel_anisotropic_sampler_key, m_pixel_anisotropic_sampler_view },
                { pixel_base_color_texture_key, m_pixel_textures_views[i] },
            });
        }
    }
}

ModelViewRenderer::~ModelViewRenderer()
{
    WaitForIdle();
}

void ModelViewRenderer::Init(const AppSize& app_size, WindowHandle window)
{
    m_swapchain = m_device->CreateSwapchain(window, app_size.width(), app_size.height(), kFrameCount, m_settings.vsync);

    glm::mat4 view = GetViewMatrix();
    glm::mat4 projection = GetProjectionMatrix(app_size.width(), app_size.height());

    std::vector<glm::mat4> vertex_constant_data(m_render_model.GetMeshCount());
    for (size_t i = 0; i < m_render_model.GetMeshCount(); ++i) {
        vertex_constant_data[i] = glm::transpose(projection * view * m_render_model.GetMesh(i).matrix);
    }
    m_vertex_constant_buffer->UpdateUploadBuffer(0, vertex_constant_data.data(),
                                                 sizeof(vertex_constant_data.front()) * vertex_constant_data.size());

    TextureDesc depth_stencil_texture_desc = {
        .type = TextureType::k2D,
        .format = gli::format::FORMAT_D32_SFLOAT_PACK32,
        .width = app_size.width(),
        .height = app_size.height(),
        .depth_or_array_layers = 1,
        .mip_levels = 1,
        .sample_count = 1,
        .usage = BindFlag::kDepthStencil,
    };
    m_depth_stencil_texture = m_device->CreateTexture(MemoryType::kDefault, depth_stencil_texture_desc);
    ViewDesc depth_stencil_view_desc = {
        .view_type = ViewType::kDepthStencil,
        .dimension = ViewDimension::kTexture2D,
    };
    m_depth_stencil_view = m_device->CreateView(m_depth_stencil_texture, depth_stencil_view_desc);

    GraphicsPipelineDesc pipeline_desc = {
        .shaders = { m_vertex_shader, m_pixel_shader },
        .layout = m_layout,
        .input = { { kPositions, "POSITION", gli::FORMAT_RGB32_SFLOAT_PACK32, sizeof(glm::vec3) },
                   { kTexcoords, "TEXCOORD", gli::FORMAT_RG32_SFLOAT_PACK32, sizeof(glm::vec2) } },
        .color_formats = { m_swapchain->GetFormat() },
        .depth_stencil_format = m_depth_stencil_texture->GetFormat(),
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
        command_list->SetViewport(0, 0, app_size.width(), app_size.height());
        command_list->SetScissorRect(0, 0, app_size.width(), app_size.height());
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kRenderTarget } });
        RenderPassDesc render_pass_desc = {
            .render_area = { 0, 0, app_size.width(), app_size.height() },
            .colors = { { .view = m_back_buffer_views[i],
                          .load_op = RenderPassLoadOp::kClear,
                          .store_op = RenderPassStoreOp::kStore,
                          .clear_value = glm::vec4(0.0, 0.2, 0.4, 1.0) } },
            .depth = { .load_op = RenderPassLoadOp::kClear,
                       .store_op = RenderPassStoreOp::kDontCare,
                       .clear_value = 1.0 },
            .depth_stencil_view = m_depth_stencil_view,
        };
        command_list->BeginRenderPass(render_pass_desc);
        for (size_t j = 0; j < m_render_model.GetMeshCount(); ++j) {
            const auto& mesh = m_render_model.GetMesh(j);
            command_list->BindBindingSet(m_binding_sets[j]);
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

void ModelViewRenderer::Resize(const AppSize& app_size, WindowHandle window)
{
    WaitForIdle();
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        m_back_buffer_views[i].reset();
    }
    m_swapchain.reset();
    Init(app_size, window);
}

void ModelViewRenderer::Render()
{
    uint32_t frame_index = m_swapchain->NextImage(m_fence, ++m_fence_value);
    m_command_queue->Wait(m_fence, m_fence_value);
    m_fence->Wait(m_fence_values[frame_index]);
    m_command_queue->ExecuteCommandLists({ m_command_lists[frame_index] });
    m_command_queue->Signal(m_fence, m_fence_values[frame_index] = ++m_fence_value);
    m_swapchain->Present(m_fence, m_fence_values[frame_index]);
}

std::string_view ModelViewRenderer::GetTitle() const
{
    return "ModelView";
}

const std::string& ModelViewRenderer::GetGpuName() const
{
    return m_adapter->GetName();
}

const Settings& ModelViewRenderer::GetSettings() const
{
    return m_settings;
}

void ModelViewRenderer::WaitForIdle()
{
    m_command_queue->Signal(m_fence, ++m_fence_value);
    m_fence->Wait(m_fence_value);
}

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppLoop::Run(std::make_unique<ModelViewRenderer>(settings), argc, argv);
}

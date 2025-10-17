#include "AppBox/AppBox.h"
#include "AppSettings/ArgsParser.h"
#include "Instance/Instance.h"
#include "RenderUtils/ModelLoader.h"
#include "RenderUtils/RenderModel.h"

namespace {

glm::mat4 GetViewMatrix()
{
    glm::vec3 eye = glm::vec3(0.0f, 0.0f, 3.5f);
    glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    return glm::lookAt(eye, center, up);
}

glm::mat4 GetProjectionMatrix(uint32_t width, uint32_t height)
{
    return glm::perspective(glm::radians(45.0f), static_cast<float>(width) / height,
                            /*zNear=*/0.1f, /*zFar=*/128.0f);
}

} // namespace

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("DesktopModelView", settings);
    AppSize app_size = app.GetAppSize();

    std::shared_ptr<Instance> instance = CreateInstance(settings.api_type);
    std::shared_ptr<Adapter> adapter = std::move(instance->EnumerateAdapters()[settings.required_gpu_index]);
    app.SetGpuName(adapter->GetName());
    std::shared_ptr<Device> device = adapter->CreateDevice();
    std::shared_ptr<CommandQueue> command_queue = device->GetCommandQueue(CommandListType::kGraphics);
    static constexpr uint32_t kFrameCount = 3;
    std::shared_ptr<Swapchain> swapchain = device->CreateSwapchain(app.GetNativeWindow(), app_size.width(),
                                                                   app_size.height(), kFrameCount, settings.vsync);
    uint64_t fence_value = 0;
    std::shared_ptr<Fence> fence = device->CreateFence(fence_value);

    std::unique_ptr<Model> model = LoadModel("resources/models/DamagedHelmet/glTF/DamagedHelmet.gltf");
    RenderModel render_model(device, command_queue, std::move(model));

    glm::mat4 view = GetViewMatrix();
    glm::mat4 projection = GetProjectionMatrix(app_size.width(), app_size.height());

    std::vector<glm::mat4> vertex_cbv_data(render_model.GetMeshCount());
    std::shared_ptr<Resource> vertex_cbv_buffer =
        device->CreateBuffer(BindFlag::kConstantBuffer, sizeof(vertex_cbv_data.front()) * vertex_cbv_data.size());
    vertex_cbv_buffer->CommitMemory(MemoryType::kUpload);
    std::vector<std::shared_ptr<View>> vertex_cbv_views(render_model.GetMeshCount());
    for (size_t i = 0; i < render_model.GetMeshCount(); ++i) {
        ViewDesc vertex_cbv_view_desc = {
            .view_type = ViewType::kConstantBuffer,
            .dimension = ViewDimension::kBuffer,
            .offset = i * sizeof(vertex_cbv_data.front()),
        };
        vertex_cbv_views[i] = device->CreateView(vertex_cbv_buffer, vertex_cbv_view_desc);
        vertex_cbv_data[i] = glm::transpose(projection * view * render_model.GetMesh(i).matrix);
    }
    vertex_cbv_buffer->UpdateUploadBuffer(0, vertex_cbv_data.data(),
                                          sizeof(vertex_cbv_data.front()) * vertex_cbv_data.size());

    std::vector<std::shared_ptr<View>> pixel_bindless_textures_views(render_model.GetMeshCount());
    for (size_t i = 0; i < render_model.GetMeshCount(); ++i) {
        ViewDesc pixel_bindless_textures_view_desc = {
            .view_type = ViewType::kTexture,
            .dimension = ViewDimension::kTexture2D,
            .bindless = true,
        };
        pixel_bindless_textures_views[i] =
            device->CreateView(render_model.GetMesh(i).textures.base_color, pixel_bindless_textures_view_desc);
    }

    std::shared_ptr<Resource> pixel_anisotropic_sampler = device->CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever,
    });
    ViewDesc pixel_anisotropic_sampler_view_desc = {
        .view_type = ViewType::kSampler,
    };
    std::shared_ptr<View> pixel_anisotropic_sampler_view =
        device->CreateView(pixel_anisotropic_sampler, pixel_anisotropic_sampler_view_desc);

    std::vector<uint32_t> pixel_cbv_data(render_model.GetMeshCount());
    std::shared_ptr<Resource> pixel_cbv_buffer =
        device->CreateBuffer(BindFlag::kConstantBuffer, sizeof(pixel_cbv_data.front()) * pixel_cbv_data.size());
    pixel_cbv_buffer->CommitMemory(MemoryType::kUpload);
    std::vector<std::shared_ptr<View>> pixel_cbv_views(render_model.GetMeshCount());
    for (size_t i = 0; i < render_model.GetMeshCount(); ++i) {
        ViewDesc pixel_cbv_view_desc = {
            .view_type = ViewType::kConstantBuffer,
            .dimension = ViewDimension::kBuffer,
            .offset = i * sizeof(pixel_cbv_data.front()),
        };
        pixel_cbv_views[i] = device->CreateView(pixel_cbv_buffer, pixel_cbv_view_desc);
        pixel_cbv_data[i] = pixel_bindless_textures_views[i]->GetDescriptorId();
    }
    pixel_cbv_buffer->UpdateUploadBuffer(0, pixel_cbv_data.data(),
                                         sizeof(pixel_cbv_data.front()) * pixel_cbv_data.size());

    std::shared_ptr<Resource> depth_stencil_texture =
        device->CreateTexture(TextureType::k2D, BindFlag::kDepthStencil, gli::format::FORMAT_D32_SFLOAT_PACK32,
                              /*sample_count=*/1, app_size.width(), app_size.height(),
                              /*depth=*/1, /*mip_levels=*/1);
    depth_stencil_texture->CommitMemory(MemoryType::kDefault);
    ViewDesc depth_stencil_view_desc = {
        .view_type = ViewType::kDepthStencil,
        .dimension = ViewDimension::kTexture2D,
    };
    std::shared_ptr<View> depth_stencil_view = device->CreateView(depth_stencil_texture, depth_stencil_view_desc);

    std::shared_ptr<Shader> vertex_shader = device->CompileShader(
        { ASSETS_PATH "shaders/ModelView/VertexShader.hlsl", "main", ShaderType::kVertex, "6_0" });
    std::shared_ptr<Shader> pixel_shader =
        device->CompileShader({ ASSETS_PATH "shaders/ModelView/PixelShader.hlsl", "main", ShaderType::kPixel, "6_0" });

    BindKey vertex_cbv_key = vertex_shader->GetBindKey("cbv");
    BindKey pixel_bindless_textures_key = pixel_shader->GetBindKey("bindless_textures");
    BindKey pixel_anisotropic_sampler_key = pixel_shader->GetBindKey("anisotropic_sampler");
    BindKey pixel_cbv_key = pixel_shader->GetBindKey("cbv");
    std::shared_ptr<BindingSetLayout> layout = device->CreateBindingSetLayout(
        { vertex_cbv_key, pixel_bindless_textures_key, pixel_anisotropic_sampler_key, pixel_cbv_key });

    RenderPassDesc render_pass_desc = {
        .colors = { { swapchain->GetFormat(), RenderPassLoadOp::kClear, RenderPassStoreOp::kStore } },
        .depth_stencil = { depth_stencil_texture->GetFormat(), RenderPassLoadOp::kClear, RenderPassStoreOp::kDontCare },
    };
    std::shared_ptr<RenderPass> render_pass = device->CreateRenderPass(render_pass_desc);

    static constexpr uint32_t kPositions = 0;
    static constexpr uint32_t kTexcoords = 1;
    std::vector<InputLayoutDesc> vextex_input = {
        { kPositions, "POSITION", gli::FORMAT_RGB32_SFLOAT_PACK32, sizeof(glm::vec3) },
        { kTexcoords, "TEXCOORD", gli::FORMAT_RG32_SFLOAT_PACK32, sizeof(glm::vec2) }
    };

    GraphicsPipelineDesc pipeline_desc = {
        device->CreateProgram({ vertex_shader, pixel_shader }),
        layout,
        vextex_input,
        render_pass,
    };
    std::shared_ptr<Pipeline> pipeline = device->CreateGraphicsPipeline(pipeline_desc);

    std::vector<std::shared_ptr<BindingSet>> binding_sets(render_model.GetMeshCount());
    for (size_t i = 0; i < render_model.GetMeshCount(); ++i) {
        binding_sets[i] = device->CreateBindingSet(layout);
        binding_sets[i]->WriteBindings({
            { vertex_cbv_key, vertex_cbv_views[i] },
            { pixel_anisotropic_sampler_key, pixel_anisotropic_sampler_view },
            { pixel_cbv_key, pixel_cbv_views[i] },
        });
    }

    std::array<uint64_t, kFrameCount> fence_values = {};
    std::array<std::shared_ptr<CommandList>, kFrameCount> command_lists;
    std::array<std::shared_ptr<View>, kFrameCount> back_buffer_views;
    std::array<std::shared_ptr<Framebuffer>, kFrameCount> framebuffers;
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        std::shared_ptr<Resource> back_buffer = swapchain->GetBackBuffer(i);
        ViewDesc back_buffer_view_desc = {
            .view_type = ViewType::kRenderTarget,
            .dimension = ViewDimension::kTexture2D,
        };
        back_buffer_views[i] = device->CreateView(back_buffer, back_buffer_view_desc);

        FramebufferDesc framebuffer_desc = {
            .render_pass = render_pass,
            .width = app_size.width(),
            .height = app_size.height(),
            .colors = { back_buffer_views[i] },
            .depth_stencil = depth_stencil_view,
        };
        framebuffers[i] = device->CreateFramebuffer(framebuffer_desc);

        auto& command_list = command_lists[i];
        command_list = device->CreateCommandList(CommandListType::kGraphics);
        command_list->BindPipeline(pipeline);
        command_list->SetViewport(0, 0, app_size.width(), app_size.height());
        command_list->SetScissorRect(0, 0, app_size.width(), app_size.height());
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kRenderTarget } });
        ClearDesc clear_desc = { .colors = { { 0.0, 0.2, 0.4, 1.0 } } };
        command_list->BeginRenderPass(render_pass, framebuffers[i], clear_desc);
        for (size_t j = 0; j < render_model.GetMeshCount(); ++j) {
            const auto& mesh = render_model.GetMesh(j);
            command_list->BindBindingSet(binding_sets[j]);
            command_list->IASetIndexBuffer(mesh.indices, mesh.index_format);
            command_list->IASetVertexBuffer(kPositions, mesh.positions);
            command_list->IASetVertexBuffer(kTexcoords, mesh.texcoords);
            command_list->DrawIndexed(mesh.index_count, 1, 0, 0, 0);
        }
        command_list->EndRenderPass();
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kRenderTarget, ResourceState::kPresent } });
        command_list->Close();
    }

    while (!app.PollEvents()) {
        uint32_t frame_index = swapchain->NextImage(fence, ++fence_value);
        command_queue->Wait(fence, fence_value);
        fence->Wait(fence_values[frame_index]);
        command_queue->ExecuteCommandLists({ command_lists[frame_index] });
        command_queue->Signal(fence, fence_values[frame_index] = ++fence_value);
        swapchain->Present(fence, fence_values[frame_index]);
    }
    command_queue->Signal(fence, ++fence_value);
    fence->Wait(fence_value);
    return 0;
}

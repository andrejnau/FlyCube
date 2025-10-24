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
    AppBox app("DesktopDepthStencilRead", settings);
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

    Model fullscreen_triangle_model = {
        .meshes = { {
            .indices = { 0, 1, 2 },
            .positions = { glm::vec3(-1, 1, 0), glm::vec3(3, 1, 0), glm::vec3(-1, -3, 0) },
            .texcoords = { glm::vec2(0, 0), glm::vec2(2, 0), glm::vec2(0, 2) },
        } },
    };
    RenderModel fullscreen_triangle_render_model(device, command_queue, &fullscreen_triangle_model);

    glm::mat4 view = GetViewMatrix();
    glm::uvec2 depth_stencil_size = glm::uvec2(app_size.width() / 2, app_size.height());
    glm::mat4 projection = GetProjectionMatrix(depth_stencil_size.x, depth_stencil_size.y);

    std::vector<glm::mat4> depth_stencil_pass_cbv_data(render_model.GetMeshCount());
    std::shared_ptr<Resource> depth_stencil_pass_cbv_buffer = device->CreateBuffer({
        .size = sizeof(depth_stencil_pass_cbv_data.front()) * depth_stencil_pass_cbv_data.size(),
        .usage = BindFlag::kConstantBuffer,
    });
    depth_stencil_pass_cbv_buffer->CommitMemory(MemoryType::kUpload);
    std::vector<std::shared_ptr<View>> depth_stencil_pass_cbv_views(render_model.GetMeshCount());
    for (size_t i = 0; i < render_model.GetMeshCount(); ++i) {
        ViewDesc depth_stencil_pass_cbv_view_desc = {
            .view_type = ViewType::kConstantBuffer,
            .dimension = ViewDimension::kBuffer,
            .offset = i * sizeof(depth_stencil_pass_cbv_data.front()),
        };
        depth_stencil_pass_cbv_views[i] =
            device->CreateView(depth_stencil_pass_cbv_buffer, depth_stencil_pass_cbv_view_desc);
        depth_stencil_pass_cbv_data[i] = glm::transpose(projection * view * render_model.GetMesh(i).matrix);
    }
    depth_stencil_pass_cbv_buffer->UpdateUploadBuffer(
        0, depth_stencil_pass_cbv_data.data(),
        sizeof(depth_stencil_pass_cbv_data.front()) * depth_stencil_pass_cbv_data.size());

    glm::mat4 vertex_cbv_data = glm::transpose(glm::mat4(1.0));
    std::shared_ptr<Resource> vertex_cbv_buffer = device->CreateBuffer({
        .size = sizeof(vertex_cbv_data),
        .usage = BindFlag::kConstantBuffer,
    });
    vertex_cbv_buffer->CommitMemory(MemoryType::kUpload);
    ViewDesc vertex_cbv_view_desc = {
        .view_type = ViewType::kConstantBuffer,
        .dimension = ViewDimension::kBuffer,
    };
    std::shared_ptr<View> vertex_cbv_view = device->CreateView(vertex_cbv_buffer, vertex_cbv_view_desc);
    vertex_cbv_buffer->UpdateUploadBuffer(0, &vertex_cbv_data, sizeof(vertex_cbv_data));

    glm::uvec2 pixel_cbv_data = glm::uvec2(app_size.width(), app_size.height());
    std::shared_ptr<Resource> pixel_cbv_buffer = device->CreateBuffer({
        .size = sizeof(pixel_cbv_data),
        .usage = BindFlag::kConstantBuffer,
    });
    pixel_cbv_buffer->CommitMemory(MemoryType::kUpload);
    pixel_cbv_buffer->UpdateUploadBuffer(0, &pixel_cbv_data, sizeof(pixel_cbv_data));
    ViewDesc pixel_cbv_view_desc = {
        .view_type = ViewType::kConstantBuffer,
        .dimension = ViewDimension::kBuffer,
    };
    std::shared_ptr<View> pixel_cbv_view = device->CreateView(pixel_cbv_buffer, pixel_cbv_view_desc);

    std::shared_ptr<Resource> depth_stencil_texture =
        device->CreateTexture(MemoryType::kDefault, {
                                                        .type = TextureType::k2D,
                                                        .format = gli::format::FORMAT_D32_SFLOAT_S8_UINT_PACK64,
                                                        .width = depth_stencil_size.x,
                                                        .height = depth_stencil_size.y,
                                                        .depth_or_array_layers = 1,
                                                        .mip_levels = 1,
                                                        .sample_count = 1,
                                                        .usage = BindFlag::kDepthStencil | BindFlag::kShaderResource,
                                                    });
    depth_stencil_texture->CommitMemory(MemoryType::kDefault);
    ViewDesc depth_stencil_view_desc = {
        .view_type = ViewType::kDepthStencil,
        .dimension = ViewDimension::kTexture2D,
    };
    std::shared_ptr<View> depth_stencil_view = device->CreateView(depth_stencil_texture, depth_stencil_view_desc);
    ViewDesc depth_read_view_desc = {
        .view_type = ViewType::kTexture,
        .dimension = ViewDimension::kTexture2D,
        .plane_slice = 0,
    };
    std::shared_ptr<View> depth_read_view = device->CreateView(depth_stencil_texture, depth_read_view_desc);
    ViewDesc stencil_read_view_desc = {
        .view_type = ViewType::kTexture,
        .dimension = ViewDimension::kTexture2D,
        .plane_slice = 1,
    };
    std::shared_ptr<View> stencil_read_view = device->CreateView(depth_stencil_texture, stencil_read_view_desc);

    std::shared_ptr<Shader> vertex_shader = device->CompileShader(
        { ASSETS_PATH "shaders/DepthStencilRead/VertexShader.hlsl", "main", ShaderType::kVertex, "6_0" });
    std::shared_ptr<Shader> pixel_shader = device->CompileShader(
        { ASSETS_PATH "shaders/DepthStencilRead/PixelShader.hlsl", "main", ShaderType::kPixel, "6_0" });

    BindKey vertex_cbv_key = vertex_shader->GetBindKey("cbv");
    BindKey pixel_cbv_key = pixel_shader->GetBindKey("cbv");
    BindKey pixel_depth_buffer_key = pixel_shader->GetBindKey("depth_buffer");
    BindKey pixel_stencil_buffer_key = pixel_shader->GetBindKey("stencil_buffer");
    std::shared_ptr<BindingSetLayout> layout = device->CreateBindingSetLayout(
        { vertex_cbv_key, pixel_cbv_key, pixel_depth_buffer_key, pixel_stencil_buffer_key });

    static constexpr uint32_t kPositions = 0;
    static constexpr uint32_t kTexcoords = 1;
    std::vector<InputLayoutDesc> vextex_input = {
        { kPositions, "POSITION", gli::FORMAT_RGB32_SFLOAT_PACK32, sizeof(glm::vec3) },
        { kTexcoords, "TEXCOORD", gli::FORMAT_RG32_SFLOAT_PACK32, sizeof(glm::vec2) }
    };

    RenderPassDesc depth_stencil_pass_render_pass_desc = {
        .depth_stencil = { depth_stencil_texture->GetFormat(), RenderPassLoadOp::kClear, RenderPassStoreOp::kStore,
                           RenderPassLoadOp::kClear, RenderPassStoreOp::kStore },
    };
    std::shared_ptr<RenderPass> depth_stencil_pass_render_pass =
        device->CreateRenderPass(depth_stencil_pass_render_pass_desc);

    DepthStencilDesc depth_stencil_pass_depth_stencil_desc = {
        .stencil_enable = true,
        .front_face = { .depth_fail_op = StencilOp::kIncr, .pass_op = StencilOp::kIncr },
        .back_face = { .depth_fail_op = StencilOp::kIncr, .pass_op = StencilOp::kIncr },
    };

    GraphicsPipelineDesc depth_stencil_pass_pipeline_desc = {
        device->CreateProgram({ vertex_shader }), layout, vextex_input, depth_stencil_pass_render_pass,
        depth_stencil_pass_depth_stencil_desc,
    };
    std::shared_ptr<Pipeline> depth_stencil_pass_pipeline =
        device->CreateGraphicsPipeline(depth_stencil_pass_pipeline_desc);

    RenderPassDesc render_pass_desc = {
        .colors = { { swapchain->GetFormat(), RenderPassLoadOp::kClear, RenderPassStoreOp::kStore } },
    };
    std::shared_ptr<RenderPass> render_pass = device->CreateRenderPass(render_pass_desc);

    DepthStencilDesc depth_stencil_desc = {
        .depth_test_enable = false,
        .depth_func = ComparisonFunc::kAlways,
        .depth_write_enable = false,
    };

    GraphicsPipelineDesc pipeline_desc = {
        device->CreateProgram({ vertex_shader, pixel_shader }), layout, vextex_input, render_pass, depth_stencil_desc,
    };
    std::shared_ptr<Pipeline> pipeline = device->CreateGraphicsPipeline(pipeline_desc);

    std::vector<std::shared_ptr<BindingSet>> depth_stencil_pass_binding_sets(render_model.GetMeshCount());
    for (size_t i = 0; i < render_model.GetMeshCount(); ++i) {
        depth_stencil_pass_binding_sets[i] = device->CreateBindingSet(layout);
        depth_stencil_pass_binding_sets[i]->WriteBindings({
            { vertex_cbv_key, depth_stencil_pass_cbv_views[i] },
        });
    }

    std::shared_ptr<BindingSet> binding_set = device->CreateBindingSet(layout);
    binding_set->WriteBindings({
        { vertex_cbv_key, vertex_cbv_view },
        { pixel_cbv_key, pixel_cbv_view },
        { pixel_depth_buffer_key, depth_read_view },
        { pixel_stencil_buffer_key, stencil_read_view },
    });

    std::array<uint64_t, kFrameCount> fence_values = {};
    std::array<std::shared_ptr<CommandList>, kFrameCount> command_lists;
    std::array<std::shared_ptr<View>, kFrameCount> back_buffer_views;
    std::array<std::shared_ptr<Framebuffer>, kFrameCount> depth_stencil_pass_framebuffers;
    std::array<std::shared_ptr<Framebuffer>, kFrameCount> framebuffers;
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        std::shared_ptr<Resource> back_buffer = swapchain->GetBackBuffer(i);
        ViewDesc back_buffer_view_desc = {
            .view_type = ViewType::kRenderTarget,
            .dimension = ViewDimension::kTexture2D,
        };
        back_buffer_views[i] = device->CreateView(back_buffer, back_buffer_view_desc);

        FramebufferDesc depth_stencil_pass_framebuffer_desc = {
            .render_pass = depth_stencil_pass_render_pass,
            .width = depth_stencil_size.x,
            .height = depth_stencil_size.y,
            .depth_stencil = depth_stencil_view,
        };
        depth_stencil_pass_framebuffers[i] = device->CreateFramebuffer(depth_stencil_pass_framebuffer_desc);

        FramebufferDesc framebuffer_desc = {
            .render_pass = render_pass,
            .width = app_size.width(),
            .height = app_size.height(),
            .colors = { back_buffer_views[i] },
        };
        framebuffers[i] = device->CreateFramebuffer(framebuffer_desc);

        auto& command_list = command_lists[i];
        command_list = device->CreateCommandList(CommandListType::kGraphics);
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kRenderTarget } });

        command_list->BindPipeline(depth_stencil_pass_pipeline);
        command_list->BeginRenderPass(depth_stencil_pass_render_pass, depth_stencil_pass_framebuffers[i],
                                      /*clear_desc=*/{});
        command_list->SetViewport(0, 0, depth_stencil_size.x, app_size.height());
        command_list->SetScissorRect(0, 0, depth_stencil_size.y, app_size.height());
        for (size_t j = 0; j < render_model.GetMeshCount(); ++j) {
            const auto& mesh = render_model.GetMesh(j);
            command_list->BindBindingSet(depth_stencil_pass_binding_sets[j]);
            command_list->IASetIndexBuffer(mesh.indices, 0, mesh.index_format);
            command_list->IASetVertexBuffer(kPositions, mesh.positions, 0);
            command_list->IASetVertexBuffer(kTexcoords, mesh.texcoords, 0);
            command_list->DrawIndexed(mesh.index_count, 1, 0, 0, 0);
        }
        command_list->EndRenderPass();

        command_list->BindPipeline(pipeline);
        command_list->BeginRenderPass(render_pass, framebuffers[i], /*clear_desc=*/{});
        command_list->SetViewport(0, 0, app_size.width(), app_size.height());
        command_list->SetScissorRect(0, 0, app_size.width(), app_size.height());
        for (size_t j = 0; j < fullscreen_triangle_render_model.GetMeshCount(); ++j) {
            const auto& mesh = fullscreen_triangle_render_model.GetMesh(j);
            command_list->BindBindingSet(binding_set);
            command_list->IASetIndexBuffer(mesh.indices, 0, mesh.index_format);
            command_list->IASetVertexBuffer(kPositions, mesh.positions, 0);
            command_list->IASetVertexBuffer(kTexcoords, mesh.texcoords, 0);
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

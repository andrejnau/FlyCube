#include "ApiType/ApiType.h"
#include "AppLoop/AppLoop.h"
#include "Instance/Instance.h"
#include "Utilities/Common.h"

#import <Foundation/Foundation.h>

class TriangleRenderer : public AppRenderer {
public:
    TriangleRenderer();
    void Init(const AppSize& app_size, WindowHandle window) override;
    void Resize(const AppSize& app_size, WindowHandle window) override;
    void Render() override;

    std::string_view GetTitle() const override
    {
        return "Triangle";
    }

    void WaitForIdle();

    ~TriangleRenderer() override;

    std::shared_ptr<Instance> instance;
    std::shared_ptr<Adapter> adapter;
    std::shared_ptr<Device> device;
    std::shared_ptr<CommandQueue> command_queue;
    std::shared_ptr<Swapchain> swapchain;
    uint64_t fence_value = 0;
    std::shared_ptr<Fence> fence;
    static constexpr uint32_t frame_count = 3;
    std::array<uint64_t, frame_count> fence_values = {};
    std::vector<std::shared_ptr<Framebuffer>> framebuffers;
    std::array<std::shared_ptr<CommandList>, frame_count> command_lists;
    std::shared_ptr<Resource> index_buffer;
    std::shared_ptr<Resource> vertex_buffer;
    std::shared_ptr<Resource> constant_buffer;
    std::shared_ptr<Shader> vertex_shader;
    std::shared_ptr<Shader> pixel_shader;
    std::shared_ptr<Program> program;
    std::shared_ptr<RenderPass> render_pass;
    std::shared_ptr<Pipeline> pipeline;
    std::shared_ptr<View> constant_view;
    std::shared_ptr<BindingSetLayout> layout;
    std::shared_ptr<BindingSet> binding_set;
};

TriangleRenderer::TriangleRenderer()
{
    instance = CreateInstance(ApiType::kMetal);
    adapter = std::move(instance->EnumerateAdapters().front());
    device = adapter->CreateDevice();
    command_queue = device->GetCommandQueue(CommandListType::kGraphics);
    fence = device->CreateFence(fence_value);
}

void TriangleRenderer::Init(const AppSize& app_size, WindowHandle window)
{
    swapchain = device->CreateSwapchain(window, app_size.width(), app_size.height(), frame_count, false);

    std::vector<uint32_t> index_data = { 0, 1, 2 };
    index_buffer =
        device->CreateBuffer(BindFlag::kIndexBuffer | BindFlag::kCopyDest, sizeof(uint32_t) * index_data.size());
    index_buffer->CommitMemory(MemoryType::kUpload);
    index_buffer->UpdateUploadBuffer(0, index_data.data(), sizeof(index_data.front()) * index_data.size());

    std::vector<glm::vec3> vertex_data = {
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3(0.0, 0.5, 0.0),
        glm::vec3(0.5, -0.5, 0.0),
    };
    vertex_buffer = device->CreateBuffer(BindFlag::kVertexBuffer | BindFlag::kCopyDest,
                                         sizeof(vertex_data.front()) * vertex_data.size());
    vertex_buffer->CommitMemory(MemoryType::kUpload);
    vertex_buffer->UpdateUploadBuffer(0, vertex_data.data(), sizeof(vertex_data.front()) * vertex_data.size());

    glm::vec4 constant_data = glm::vec4(1, 0, 0, 1);
    constant_buffer = device->CreateBuffer(BindFlag::kConstantBuffer | BindFlag::kCopyDest, sizeof(constant_data));
    constant_buffer->CommitMemory(MemoryType::kUpload);
    constant_buffer->UpdateUploadBuffer(0, &constant_data, sizeof(constant_data));

    auto vertex_path = [[NSBundle mainBundle] pathForResource:@"VertexShader" ofType:@"spv"];
    auto pixel_path = [[NSBundle mainBundle] pathForResource:@"PixelShader" ofType:@"spv"];
    vertex_shader =
        device->CreateShader(ReadBinaryFile([vertex_path UTF8String]), ShaderBlobType::kSPIRV, ShaderType::kVertex);
    pixel_shader =
        device->CreateShader(ReadBinaryFile([pixel_path UTF8String]), ShaderBlobType::kSPIRV, ShaderType::kPixel);
    program = device->CreateProgram({ vertex_shader, pixel_shader });

    ViewDesc constant_view_desc = {};
    constant_view_desc.view_type = ViewType::kConstantBuffer;
    constant_view_desc.dimension = ViewDimension::kBuffer;
    constant_view = device->CreateView(constant_buffer, constant_view_desc);
    BindKey settings_key = { ShaderType::kPixel, ViewType::kConstantBuffer, 0, 0, 1 };
    layout = device->CreateBindingSetLayout({ settings_key });
    binding_set = device->CreateBindingSet(layout);
    binding_set->WriteBindings({ { settings_key, constant_view } });

    RenderPassDesc render_pass_desc = {
        { { swapchain->GetFormat(), RenderPassLoadOp::kClear, RenderPassStoreOp::kStore } },
    };
    render_pass = device->CreateRenderPass(render_pass_desc);
    ClearDesc clear_desc = { { { 0.0, 0.2, 0.4, 1.0 } } };
    GraphicsPipelineDesc pipeline_desc = {
        program,
        layout,
        { { 0, "POSITION", gli::FORMAT_RGB32_SFLOAT_PACK32, sizeof(vertex_data.front()) } },
        render_pass,
    };
    pipeline = device->CreateGraphicsPipeline(pipeline_desc);

    for (uint32_t i = 0; i < frame_count; ++i) {
        ViewDesc back_buffer_view_desc = {};
        back_buffer_view_desc.view_type = ViewType::kRenderTarget;
        back_buffer_view_desc.dimension = ViewDimension::kTexture2D;
        std::shared_ptr<Resource> back_buffer = swapchain->GetBackBuffer(i);
        std::shared_ptr<View> back_buffer_view = device->CreateView(back_buffer, back_buffer_view_desc);
        FramebufferDesc framebuffer_desc = {};
        framebuffer_desc.render_pass = render_pass;
        framebuffer_desc.width = app_size.width();
        framebuffer_desc.height = app_size.height();
        framebuffer_desc.colors = { back_buffer_view };
        std::shared_ptr<Framebuffer> framebuffer =
            framebuffers.emplace_back(device->CreateFramebuffer(framebuffer_desc));
        decltype(auto) command_list = command_lists[i];
        command_list = device->CreateCommandList(CommandListType::kGraphics);
        command_list->BindPipeline(pipeline);
        command_list->BindBindingSet(binding_set);
        command_list->SetViewport(0, 0, app_size.width(), app_size.height());
        command_list->SetScissorRect(0, 0, app_size.width(), app_size.height());
        command_list->IASetIndexBuffer(index_buffer, gli::format::FORMAT_R32_UINT_PACK32);
        command_list->IASetVertexBuffer(0, vertex_buffer);
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kRenderTarget } });
        command_list->BeginRenderPass(render_pass, framebuffer, clear_desc);
        command_list->DrawIndexed(3, 1, 0, 0, 0);
        command_list->EndRenderPass();
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kRenderTarget, ResourceState::kPresent } });
        command_list->Close();
    }
}

void TriangleRenderer::Resize(const AppSize& app_size, WindowHandle window)
{
    WaitForIdle();
    Init(app_size, window);
}

void TriangleRenderer::Render()
{
    uint32_t frame_index = swapchain->NextImage(fence, ++fence_value);
    command_queue->Wait(fence, fence_value);
    fence->Wait(fence_values[frame_index]);
    command_queue->ExecuteCommandLists({ command_lists[frame_index] });
    command_queue->Signal(fence, fence_values[frame_index] = ++fence_value);
    swapchain->Present(fence, fence_values[frame_index]);
}

void TriangleRenderer::WaitForIdle()
{
    command_queue->Signal(fence, ++fence_value);
    fence->Wait(fence_value);
}

TriangleRenderer::~TriangleRenderer()
{
    WaitForIdle();
}

int main(int argc, char* argv[])
{
    AppLoop::Run(std::make_unique<TriangleRenderer>(), argc, argv);
}

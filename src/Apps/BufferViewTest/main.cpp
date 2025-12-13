#include "ApiType/ApiType.h"
#include "AppLoop/AppLoop.h"
#include "AppSettings/ArgsParser.h"
#include "Instance/Instance.h"
#include "Utilities/Asset.h"
#include "Utilities/Check.h"
#include "Utilities/NotReached.h"

#include <array>
#include <string>

namespace {

constexpr uint32_t kFrameCount = 3;
constexpr uint32_t kIndexCount = 64;

uint32_t MakeU32(uint32_t index)
{
    uint32_t b0 = index * 4 + 0;
    uint32_t b1 = index * 4 + 1;
    uint32_t b2 = index * 4 + 2;
    uint32_t b3 = index * 4 + 3;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

std::string GetBufferPrefix(ViewType view_type)
{
    switch (view_type) {
    case ViewType::kBuffer:
        return "buffer";
    case ViewType::kRWBuffer:
        return "rwbuffer";
    case ViewType::kStructuredBuffer:
        return "structured_buffer";
    case ViewType::kRWStructuredBuffer:
        return "rwstructured_buffer";
    case ViewType::kByteAddressBuffer:
        return "byte_address_buffer";
    case ViewType::kRWByteAddressBuffer:
        return "rwbyte_address_buffer";
    default:
        NOTREACHED();
    }
}

bool IsSupported(ViewType view_type, gli::format format)
{
    if (view_type == ViewType::kByteAddressBuffer || view_type == ViewType::kRWByteAddressBuffer) {
        return format == gli::format::FORMAT_R32_UINT_PACK32;
    }
    if (format == gli::format::FORMAT_RGB32_UINT_PACK32) {
        return view_type != ViewType::kBuffer && view_type != ViewType::kRWBuffer;
    }
    return true;
}

} // namespace

class BufferViewTestRenderer : public AppRenderer {
public:
    explicit BufferViewTestRenderer(const Settings& settings);
    ~BufferViewTestRenderer() override;

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
    std::shared_ptr<Shader> vertex_shader_;
    std::shared_ptr<Shader> pixel_shader_;
    std::shared_ptr<CommandList> upload_command_list_;
    std::vector<std::shared_ptr<Resource>> upload_buffers_;
    std::vector<std::shared_ptr<Resource>> buffers_;
    std::vector<std::shared_ptr<View>> buffer_views_;
    std::shared_ptr<BindingSetLayout> layout_;
    std::shared_ptr<BindingSet> binding_set_;

    std::shared_ptr<Swapchain> swapchain_;
    std::shared_ptr<Pipeline> pipeline_;
    std::array<std::shared_ptr<View>, kFrameCount> back_buffer_views_ = {};
    std::array<std::shared_ptr<CommandList>, kFrameCount> command_lists_ = {};
    std::array<uint64_t, kFrameCount> fence_values_ = {};
};

BufferViewTestRenderer::BufferViewTestRenderer(const Settings& settings)
    : settings_(settings)
{
    instance_ = CreateInstance(settings_.api_type);
    adapter_ = std::move(instance_->EnumerateAdapters()[settings_.required_gpu_index]);
    device_ = adapter_->CreateDevice();
    command_queue_ = device_->GetCommandQueue(CommandListType::kGraphics);
    fence_ = device_->CreateFence(fence_value_);

    ShaderBlobType blob_type = device_->GetSupportedShaderBlobType();
    std::vector<uint8_t> vertex_blob = AssetLoadShaderBlob("assets/BufferViewTest/VertexShader.hlsl", blob_type);
    std::vector<uint8_t> pixel_blob = AssetLoadShaderBlob("assets/BufferViewTest/PixelShader.hlsl", blob_type);
    vertex_shader_ = device_->CreateShader(vertex_blob, blob_type, ShaderType::kVertex);
    pixel_shader_ = device_->CreateShader(pixel_blob, blob_type, ShaderType::kPixel);

    upload_command_list_ = device_->CreateCommandList(CommandListType::kGraphics);

    BindingSetLayoutDesc binding_set_layout_desc = {};
    WriteBindingsDesc write_bindings_desc = {};
    const auto view_types = std::to_array({
        ViewType::kBuffer,
        ViewType::kRWBuffer,
        ViewType::kStructuredBuffer,
        ViewType::kRWStructuredBuffer,
        ViewType::kByteAddressBuffer,
        ViewType::kRWByteAddressBuffer,
    });
    const auto formats = std::to_array({
        gli::format::FORMAT_R32_UINT_PACK32,
        gli::format::FORMAT_RG32_UINT_PACK32,
        gli::format::FORMAT_RGB32_UINT_PACK32,
        gli::format::FORMAT_RGBA32_UINT_PACK32,
    });

    for (const auto& view_type : view_types) {
        for (const auto& format : formats) {
            if (!IsSupported(view_type, format)) {
                continue;
            }

            uint32_t structure_stride = gli::detail::bits_per_pixel(format) / 8;
            BufferDesc buffer_desc = {
                .size = sizeof(uint32_t) * kIndexCount,
            };
            if (buffer_desc.size % structure_stride != 0) {
                buffer_desc.size += structure_stride - (buffer_desc.size % structure_stride);
            }
            if (view_type == ViewType::kBuffer || view_type == ViewType::kStructuredBuffer ||
                view_type == ViewType::kByteAddressBuffer) {
                buffer_desc.usage = BindFlag::kShaderResource;
            } else {
                buffer_desc.usage = BindFlag::kCopySource;
            }

            std::shared_ptr<Resource> buffer = device_->CreateBuffer(MemoryType::kUpload, buffer_desc);
            uint32_t* data = reinterpret_cast<uint32_t*>(buffer->Map());
            for (uint32_t i = 0; i < kIndexCount; ++i) {
                data[i] = MakeU32(i);
            }
            buffer->Unmap();

            if (view_type == ViewType::kRWBuffer || view_type == ViewType::kRWStructuredBuffer ||
                view_type == ViewType::kRWByteAddressBuffer) {
                upload_buffers_.push_back(std::move(buffer));
                buffer_desc.usage = BindFlag::kUnorderedAccess | BindFlag::kCopyDest;
                buffer = device_->CreateBuffer(MemoryType::kDefault, buffer_desc);

                BufferCopyRegion copy_region = {
                    .src_offset = 0,
                    .dst_offset = 0,
                    .num_bytes = buffer_desc.size,
                };
                upload_command_list_->CopyBuffer(upload_buffers_.back(), buffer, { copy_region });
            }

            ViewDesc buffer_view_desc = {
                .view_type = view_type,
                .dimension = ViewDimension::kBuffer,
            };
            if (view_type == ViewType::kBuffer || view_type == ViewType::kRWBuffer) {
                buffer_view_desc.buffer_format = format;
            } else {
                buffer_view_desc.structure_stride = structure_stride;
            }
            std::shared_ptr<View> buffer_view = device_->CreateView(buffer, buffer_view_desc);

            std::string bind_key_name = GetBufferPrefix(view_type);
            if (view_type != ViewType::kByteAddressBuffer && view_type != ViewType::kRWByteAddressBuffer) {
                bind_key_name += "_uint" + std::to_string(structure_stride / 4);
            }
            BindKey bind_key = pixel_shader_->GetBindKey(bind_key_name);
            DCHECK(bind_key.view_type == view_type);
            binding_set_layout_desc.bind_keys.push_back(bind_key);
            write_bindings_desc.bindings.emplace_back(bind_key, buffer_view);

            buffers_.push_back(std::move(buffer));
            buffer_views_.push_back(std::move(buffer_view));
        }
    }

    upload_command_list_->Close();
    command_queue_->ExecuteCommandLists({ upload_command_list_ });

    layout_ = device_->CreateBindingSetLayout(binding_set_layout_desc);
    binding_set_ = device_->CreateBindingSet(layout_);
    binding_set_->WriteBindings(write_bindings_desc);
}

BufferViewTestRenderer::~BufferViewTestRenderer()
{
    WaitForIdle();
}

void BufferViewTestRenderer::Init(const AppSize& app_size, const NativeSurface& surface)
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

void BufferViewTestRenderer::Resize(const AppSize& app_size, const NativeSurface& surface)
{
    WaitForIdle();
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        back_buffer_views_[i].reset();
    }
    swapchain_.reset();
    Init(app_size, surface);
}

void BufferViewTestRenderer::Render()
{
    uint32_t frame_index = swapchain_->NextImage(fence_, ++fence_value_);
    command_queue_->Wait(fence_, fence_value_);
    fence_->Wait(fence_values_[frame_index]);
    command_queue_->ExecuteCommandLists({ command_lists_[frame_index] });
    command_queue_->Signal(fence_, fence_values_[frame_index] = ++fence_value_);
    swapchain_->Present(fence_, fence_values_[frame_index]);
}

std::string_view BufferViewTestRenderer::GetTitle() const
{
    return "BufferViewTest";
}

const std::string& BufferViewTestRenderer::GetGpuName() const
{
    return adapter_->GetName();
}

const Settings& BufferViewTestRenderer::GetSettings() const
{
    return settings_;
}

void BufferViewTestRenderer::WaitForIdle()
{
    command_queue_->Signal(fence_, ++fence_value_);
    fence_->Wait(fence_value_);
}

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppLoop::Run(std::make_unique<BufferViewTestRenderer>(settings), argc, argv);
}

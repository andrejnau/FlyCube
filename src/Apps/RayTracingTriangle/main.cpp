#include "AppLoop/AppLoop.h"
#include "AppSettings/ArgsParser.h"
#include "Instance/Instance.h"
#include "Utilities/Asset.h"
#include "Utilities/Check.h"
#include "Utilities/Common.h"

#include <glm/gtx/transform.hpp>

#include <cstring>

namespace {

constexpr uint32_t kFrameCount = 3;
constexpr uint32_t kNumRayQueryThreads = 8;

enum class RayTracingMode {
    kAuto,
    kForceRayTracing,
    kForceRayQuery,
};

constexpr RayTracingMode kRayTracingMode = RayTracingMode::kAuto;

constexpr bool AllowRayTracing()
{
    return kRayTracingMode == RayTracingMode::kAuto || kRayTracingMode == RayTracingMode::kForceRayTracing;
}

constexpr bool AllowRayQuery()
{
    return kRayTracingMode == RayTracingMode::kAuto || kRayTracingMode == RayTracingMode::kForceRayQuery;
}

} // namespace

class RayTracingTriangleRenderer : public AppRenderer {
public:
    explicit RayTracingTriangleRenderer(const Settings& settings);
    ~RayTracingTriangleRenderer() override;

    void Init(const NativeSurface& surface, uint32_t width, uint32_t height) override;
    void Resize(const NativeSurface& surface, uint32_t width, uint32_t height) override;
    void Render() override;
    std::string_view GetTitle() const override;
    const std::string& GetGpuName() const override;
    const Settings& GetSettings() const override;

private:
    void InitAccelerationStructures();
    void InitBindingSetLayout();
    void InitBindingSet();
    void WaitForIdle();

    Settings settings_;
    std::shared_ptr<Instance> instance_;
    std::shared_ptr<Adapter> adapter_;
    std::shared_ptr<Device> device_;
    bool use_ray_tracing_ = true;
    std::shared_ptr<CommandQueue> command_queue_;
    uint64_t fence_value_ = 0;
    std::shared_ptr<Fence> fence_;
    std::shared_ptr<Resource> acceleration_structures_memory_;
    std::shared_ptr<Resource> blas_;
    std::shared_ptr<Resource> blas_compacted_;
    std::shared_ptr<Resource> tlas_;
    std::shared_ptr<View> tlas_view_;
    std::shared_ptr<Shader> library_;
    std::shared_ptr<Shader> library_hit_;
    std::shared_ptr<Shader> library_callable_;
    std::shared_ptr<Shader> compute_shader_;
    std::shared_ptr<BindingSetLayout> layout_;
    std::shared_ptr<Pipeline> pipeline_;
    std::shared_ptr<Resource> shader_table_;
    RayTracingShaderTables shader_tables_;

    std::shared_ptr<Swapchain> swapchain_;
    std::shared_ptr<Resource> result_texture_;
    std::shared_ptr<View> result_texture_view_;
    std::shared_ptr<BindingSet> binding_set_;
    std::array<std::shared_ptr<CommandList>, kFrameCount> command_lists_ = {};
    std::array<uint64_t, kFrameCount> fence_values_ = {};
};

RayTracingTriangleRenderer::RayTracingTriangleRenderer(const Settings& settings)
    : settings_(settings)
{
    instance_ = CreateInstance(settings_.api_type);
    adapter_ = std::move(instance_->EnumerateAdapters()[settings_.required_gpu_index]);
    device_ = adapter_->CreateDevice();
    CHECK(device_->IsDxrSupported() && AllowRayTracing() || device_->IsRayQuerySupported() && AllowRayQuery(),
          "Ray Tracing is not supported");
    use_ray_tracing_ = device_->IsDxrSupported() && AllowRayTracing();
    command_queue_ = device_->GetCommandQueue(CommandListType::kGraphics);
    fence_ = device_->CreateFence(fence_value_);

    InitAccelerationStructures();

    if (use_ray_tracing_) {
        ShaderBlobType blob_type = device_->GetSupportedShaderBlobType();
        std::vector<uint8_t> library_blob = AssetLoadShaderBlob("assets/RayTracingTriangle/RayTracing.hlsl", blob_type);
        library_ = device_->CreateShader(library_blob, blob_type, ShaderType::kLibrary);
        std::vector<uint8_t> library_hit_blob =
            AssetLoadShaderBlob("assets/RayTracingTriangle/RayTracingHit.hlsl", blob_type);
        library_hit_ = device_->CreateShader(library_hit_blob, blob_type, ShaderType::kLibrary);
        std::vector<uint8_t> library_callable_blob =
            AssetLoadShaderBlob("assets/RayTracingTriangle/RayTracingCallable.hlsl", blob_type);
        library_callable_ = device_->CreateShader(library_callable_blob, blob_type, ShaderType::kLibrary);

        std::vector<RayTracingShaderGroup> groups = {
            { RayTracingShaderGroupType::kGeneral, library_->GetId("ray_gen") },
            { RayTracingShaderGroupType::kGeneral, library_->GetId("miss") },
            { RayTracingShaderGroupType::kTrianglesHitGroup, 0, library_hit_->GetId("closest_red") },
            { RayTracingShaderGroupType::kTrianglesHitGroup, 0, library_hit_->GetId("closest_green") },
            { RayTracingShaderGroupType::kGeneral, library_callable_->GetId("callable") },
        };

        InitBindingSetLayout();
        RayTracingPipelineDesc pipeline_desc = {
            .shaders = { library_, library_hit_, library_callable_ },
            .layout = layout_,
            .groups = groups,
        };
        pipeline_ = device_->CreateRayTracingPipeline(pipeline_desc);

        shader_table_ = device_->CreateBuffer(
            MemoryType::kUpload,
            { .size = device_->GetShaderTableAlignment() * groups.size(), .usage = BindFlag::kShaderTable });
        shader_table_->SetName("shader_table");

        std::vector<uint8_t> shader_handles = pipeline_->GetRayTracingShaderGroupHandles(0, groups.size());
        for (size_t i = 0; i < groups.size(); ++i) {
            shader_table_->UpdateUploadBuffer(i * device_->GetShaderTableAlignment(),
                                              shader_handles.data() + i * device_->GetShaderGroupHandleSize(),
                                              device_->GetShaderGroupHandleSize());
        }

        shader_tables_ = { .raygen = { shader_table_, 0 * device_->GetShaderTableAlignment(),
                                       device_->GetShaderTableAlignment(), device_->GetShaderTableAlignment() },
                           .miss = { shader_table_, 1 * device_->GetShaderTableAlignment(),
                                     device_->GetShaderTableAlignment(), device_->GetShaderTableAlignment() },
                           .hit = { shader_table_, 2 * device_->GetShaderTableAlignment(),
                                    2 * device_->GetShaderTableAlignment(), device_->GetShaderTableAlignment() },
                           .callable = { shader_table_, 4 * device_->GetShaderTableAlignment(),
                                         device_->GetShaderTableAlignment(), device_->GetShaderTableAlignment() } };
    } else {
        ShaderBlobType blob_type = device_->GetSupportedShaderBlobType();
        std::vector<uint8_t> compute_blob = AssetLoadShaderBlob("assets/RayTracingTriangle/RayQuery.hlsl", blob_type);
        compute_shader_ = device_->CreateShader(compute_blob, blob_type, ShaderType::kCompute);

        InitBindingSetLayout();
        ComputePipelineDesc pipeline_desc = {
            .shader = compute_shader_,
            .layout = layout_,
        };
        pipeline_ = device_->CreateComputePipeline(pipeline_desc);
    }
}

void RayTracingTriangleRenderer::InitAccelerationStructures()
{
    std::vector<uint32_t> index_data = { 0, 1, 2 };
    std::shared_ptr<Resource> index_buffer =
        device_->CreateBuffer(MemoryType::kDefault, { .size = sizeof(index_data.front()) * index_data.size(),
                                                      .usage = BindFlag::kIndexBuffer | BindFlag::kCopyDest });
    index_buffer->SetName("index_buffer");

    std::vector<glm::vec3> vertex_data = {
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3(0.0, 0.5, 0.0),
        glm::vec3(0.5, -0.5, 0.0),
    };
    std::shared_ptr<Resource> vertex_buffer =
        device_->CreateBuffer(MemoryType::kDefault, { .size = sizeof(vertex_data.front()) * vertex_data.size(),
                                                      .usage = BindFlag::kVertexBuffer | BindFlag::kCopyDest });
    vertex_buffer->SetName("vertex_buffer");

    std::shared_ptr<Resource> upload_buffer = device_->CreateBuffer(
        MemoryType::kUpload,
        { .size = index_buffer->GetWidth() + vertex_buffer->GetWidth(), .usage = BindFlag::kCopySource });
    upload_buffer->SetName("upload_buffer");
    upload_buffer->UpdateUploadBuffer(0, index_data.data(), sizeof(index_data.front()) * index_data.size());
    upload_buffer->UpdateUploadBuffer(index_buffer->GetWidth(), vertex_data.data(),
                                      sizeof(vertex_data.front()) * vertex_data.size());

    RaytracingGeometryDesc raytracing_geometry_desc = {
        { vertex_buffer, gli::format::FORMAT_RGB32_SFLOAT_PACK32, 3 },
        { index_buffer, gli::format::FORMAT_R32_UINT_PACK32, 3 },
        RaytracingGeometryFlags::kOpaque,
    };

    static constexpr uint32_t kInstanceCount = 2;
    RaytracingASPrebuildInfo blas_prebuild_info =
        device_->GetBLASPrebuildInfo({ raytracing_geometry_desc }, BuildAccelerationStructureFlags::kAllowCompaction);
    RaytracingASPrebuildInfo tlas_prebuild_info =
        device_->GetTLASPrebuildInfo(kInstanceCount, BuildAccelerationStructureFlags::kNone);

    uint64_t acceleration_structures_size = 0;
    auto add_acceleration_structure = [&](uint64_t size) {
        acceleration_structures_size = Align(acceleration_structures_size, kAccelerationStructureAlignment);
        uint64_t offset = acceleration_structures_size;
        acceleration_structures_size += size;
        return offset;
    };
    uint64_t blas_offset = add_acceleration_structure(blas_prebuild_info.acceleration_structure_size);
    uint64_t blas_compacted_offset = add_acceleration_structure(blas_prebuild_info.acceleration_structure_size);
    uint64_t tlas_offset = add_acceleration_structure(tlas_prebuild_info.acceleration_structure_size);

    acceleration_structures_memory_ = device_->CreateBuffer(
        MemoryType::kDefault, { .size = acceleration_structures_size, .usage = BindFlag::kAccelerationStructure });
    acceleration_structures_memory_->SetName("acceleration_structures_memory");

    blas_ = device_->CreateAccelerationStructure({
        .type = AccelerationStructureType::kBottomLevel,
        .buffer = acceleration_structures_memory_,
        .buffer_offset = blas_offset,
        .size = blas_prebuild_info.acceleration_structure_size,
    });

    std::shared_ptr<Resource> scratch = device_->CreateBuffer(
        MemoryType::kDefault,
        { .size = std::max(blas_prebuild_info.build_scratch_data_size, tlas_prebuild_info.build_scratch_data_size),
          .usage = BindFlag::kRayTracing });
    scratch->SetName("scratch");

    std::shared_ptr<QueryHeap> query_heap =
        device_->CreateQueryHeap(QueryHeapType::kAccelerationStructureCompactedSize, 1);

    std::shared_ptr<Resource> blas_compacted_size_buffer =
        device_->CreateBuffer(MemoryType::kReadback, { .size = sizeof(uint64_t), .usage = BindFlag::kCopyDest });
    blas_compacted_size_buffer->SetName("blas_compacted_size_buffer");

    std::shared_ptr<CommandList> upload_command_list = device_->CreateCommandList(CommandListType::kGraphics);
    upload_command_list->CopyBuffer(upload_buffer, index_buffer, { { 0, 0, index_buffer->GetWidth() } });
    upload_command_list->CopyBuffer(upload_buffer, vertex_buffer,
                                    { { index_buffer->GetWidth(), 0, vertex_buffer->GetWidth() } });
    upload_command_list->ResourceBarrier(
        { { index_buffer, ResourceState::kCopyDest, ResourceState::kNonPixelShaderResource } });
    upload_command_list->ResourceBarrier(
        { { vertex_buffer, ResourceState::kCopyDest, ResourceState::kNonPixelShaderResource } });
    upload_command_list->BuildBottomLevelAS({}, blas_, scratch, 0, { raytracing_geometry_desc },
                                            BuildAccelerationStructureFlags::kAllowCompaction);
    upload_command_list->UAVResourceBarrier(blas_);
    upload_command_list->WriteAccelerationStructuresProperties({ blas_ }, query_heap, 0);
    upload_command_list->ResolveQueryData(query_heap, 0, 1, blas_compacted_size_buffer, 0);
    upload_command_list->Close();

    command_queue_->ExecuteCommandLists({ upload_command_list });
    command_queue_->Signal(fence_, ++fence_value_);
    fence_->Wait(fence_value_);

    uint64_t blas_compacted_size = *reinterpret_cast<uint64_t*>(blas_compacted_size_buffer->Map());
    assert(blas_compacted_size != 0);
    blas_compacted_size_buffer->Unmap();

    blas_compacted_ = device_->CreateAccelerationStructure({
        .type = AccelerationStructureType::kBottomLevel,
        .buffer = acceleration_structures_memory_,
        .buffer_offset = blas_compacted_offset,
        .size = blas_compacted_size,
    });

    auto geometry = std::to_array<std::pair<std::shared_ptr<Resource>, glm::mat3x4>>({
        { blas_, glm::mat3x4(glm::transpose(glm::translate(glm::vec3(-0.5, 0.0, 0.0)))) },
        { blas_compacted_, glm::mat3x4(glm::transpose(glm::translate(glm::vec3(0.5, 0.0, 0.0)))) },
    });
    static_assert(geometry.size() == kInstanceCount);
    std::array<RaytracingGeometryInstance, kInstanceCount> instances = {};
    for (size_t i = 0; i < geometry.size(); ++i) {
        const auto& [resource, transform] = geometry[i];
        RaytracingGeometryInstance& instance = instances[i];
        memcpy(&instance.transform, &transform, sizeof(instance.transform));
        instance.instance_id = i;
        instance.instance_mask = 0xff;
        instance.instance_offset = i;
        instance.flags = RaytracingInstanceFlags::kNone;
        instance.acceleration_structure_handle = resource->GetAccelerationStructureHandle();
    }

    std::shared_ptr<Resource> instance_data = device_->CreateBuffer(
        MemoryType::kUpload, { .size = instances.size() * sizeof(instances.back()), .usage = BindFlag::kRayTracing });
    instance_data->SetName("instance_data");
    instance_data->UpdateUploadBuffer(0, instances.data(), instances.size() * sizeof(instances.back()));

    tlas_ = device_->CreateAccelerationStructure({
        .type = AccelerationStructureType::kTopLevel,
        .buffer = acceleration_structures_memory_,
        .buffer_offset = tlas_offset,
        .size = tlas_prebuild_info.acceleration_structure_size,
    });
    ViewDesc tlas_view_desc = {
        .view_type = ViewType::kAccelerationStructure,
    };
    tlas_view_ = device_->CreateView(tlas_, tlas_view_desc);

    upload_command_list->Reset();
    upload_command_list->CopyAccelerationStructure(blas_, blas_compacted_, CopyAccelerationStructureMode::kCompact);
    upload_command_list->BuildTopLevelAS({}, tlas_, scratch, 0, instance_data, 0, instances.size(),
                                         BuildAccelerationStructureFlags::kNone);
    upload_command_list->Close();

    command_queue_->ExecuteCommandLists({ upload_command_list });
    command_queue_->Signal(fence_, ++fence_value_);
    fence_->Wait(fence_value_);
}

void RayTracingTriangleRenderer::InitBindingSetLayout()
{
    const auto& shader = use_ray_tracing_ ? library_ : compute_shader_;
    BindKey geometry_key = shader->GetBindKey("geometry");
    BindKey result_texture_key = shader->GetBindKey("result_texture");
    layout_ = device_->CreateBindingSetLayout({ .bind_keys = { geometry_key, result_texture_key } });
}

void RayTracingTriangleRenderer::InitBindingSet()
{
    const auto& shader = use_ray_tracing_ ? library_ : compute_shader_;
    BindKey geometry_key = shader->GetBindKey("geometry");
    BindKey result_texture_key = shader->GetBindKey("result_texture");
    binding_set_ = device_->CreateBindingSet(layout_);
    binding_set_->WriteBindings(
        { .bindings = { { geometry_key, tlas_view_ }, { result_texture_key, result_texture_view_ } } });
}

RayTracingTriangleRenderer::~RayTracingTriangleRenderer()
{
    WaitForIdle();
}

void RayTracingTriangleRenderer::Init(const NativeSurface& surface, uint32_t width, uint32_t height)
{
    swapchain_ = device_->CreateSwapchain(surface, width, height, kFrameCount, settings_.vsync);

    TextureDesc result_texture_desc = {
        .type = TextureType::k2D,
        .format = swapchain_->GetFormat(),
        .width = width,
        .height = height,
        .depth_or_array_layers = 1,
        .mip_levels = 1,
        .sample_count = 1,
        .usage = BindFlag::kUnorderedAccess | BindFlag::kCopySource,
    };
    result_texture_ = device_->CreateTexture(MemoryType::kDefault, result_texture_desc);
    result_texture_->SetName("result_texture");

    ViewDesc result_texture_view_desc = {
        .view_type = ViewType::kRWTexture,
        .dimension = ViewDimension::kTexture2D,
    };
    result_texture_view_ = device_->CreateView(result_texture_, result_texture_view_desc);

    InitBindingSet();

    for (uint32_t i = 0; i < kFrameCount; ++i) {
        std::shared_ptr<Resource> back_buffer = swapchain_->GetBackBuffer(i);
        auto& command_list = command_lists_[i];
        command_list = device_->CreateCommandList(CommandListType::kGraphics);
        command_list->BindPipeline(pipeline_);
        command_list->BindBindingSet(binding_set_);
        if (use_ray_tracing_) {
            command_list->DispatchRays(shader_tables_, width, height, 1);
        } else {
            command_list->Dispatch((width + kNumRayQueryThreads - 1) / kNumRayQueryThreads,
                                   (height + kNumRayQueryThreads - 1) / kNumRayQueryThreads, 1);
        }
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kCopyDest } });
        command_list->ResourceBarrier(
            { { result_texture_, ResourceState::kUnorderedAccess, ResourceState::kCopySource } });
        command_list->CopyTexture(result_texture_, back_buffer, { { width, height, 1 } });
        command_list->ResourceBarrier(
            { { result_texture_, ResourceState::kCopySource, ResourceState::kUnorderedAccess } });
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kCopyDest, ResourceState::kPresent } });
        command_list->Close();
    }
}

void RayTracingTriangleRenderer::Resize(const NativeSurface& surface, uint32_t width, uint32_t height)
{
    WaitForIdle();
    swapchain_.reset();
    Init(surface, width, height);
}

void RayTracingTriangleRenderer::Render()
{
    uint32_t frame_index = swapchain_->NextImage(fence_, ++fence_value_);
    command_queue_->Wait(fence_, fence_value_);
    fence_->Wait(fence_values_[frame_index]);
    command_queue_->ExecuteCommandLists({ command_lists_[frame_index] });
    command_queue_->Signal(fence_, fence_values_[frame_index] = ++fence_value_);
    swapchain_->Present(fence_, fence_values_[frame_index]);
}

std::string_view RayTracingTriangleRenderer::GetTitle() const
{
    return "RayTracingTriangle";
}

const std::string& RayTracingTriangleRenderer::GetGpuName() const
{
    return adapter_->GetName();
}

const Settings& RayTracingTriangleRenderer::GetSettings() const
{
    return settings_;
}

void RayTracingTriangleRenderer::WaitForIdle()
{
    command_queue_->Signal(fence_, ++fence_value_);
    fence_->Wait(fence_value_);
}

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppLoop::Run(std::make_unique<RayTracingTriangleRenderer>(settings), argc, argv);
}

#include "ApiType/ApiType.h"
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

} // namespace

class RayTracingTriangleRenderer : public AppRenderer {
public:
    RayTracingTriangleRenderer(const Settings& settings);
    ~RayTracingTriangleRenderer() override;

    void Init(const AppSize& app_size, const NativeSurface& surface) override;
    void Resize(const AppSize& app_size, const NativeSurface& surface) override;
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
    std::shared_ptr<Resource> m_acceleration_structures_memory;
    std::shared_ptr<Resource> m_tlas;
    std::shared_ptr<View> m_tlas_view;
    std::shared_ptr<Shader> m_library;
    std::shared_ptr<Shader> m_library_hit;
    std::shared_ptr<Shader> m_library_callable;
    std::shared_ptr<BindingSetLayout> m_layout;
    std::shared_ptr<Pipeline> m_pipeline;
    std::shared_ptr<Resource> m_shader_table;
    RayTracingShaderTables m_shader_tables;

    std::shared_ptr<Swapchain> m_swapchain;
    std::shared_ptr<Resource> m_result_texture;
    std::shared_ptr<View> m_result_texture_view;
    std::array<std::shared_ptr<CommandList>, kFrameCount> m_command_lists = {};
    std::array<uint64_t, kFrameCount> m_fence_values = {};
};

RayTracingTriangleRenderer::RayTracingTriangleRenderer(const Settings& settings)
    : m_settings(settings)
{
    m_instance = CreateInstance(m_settings.api_type);
    m_adapter = std::move(m_instance->EnumerateAdapters()[m_settings.required_gpu_index]);
    m_device = m_adapter->CreateDevice();
    CHECK(m_device->IsDxrSupported(), "Ray Tracing is not supported");
    m_command_queue = m_device->GetCommandQueue(CommandListType::kGraphics);
    m_fence = m_device->CreateFence(m_fence_value);

    std::shared_ptr<CommandQueue> upload_command_queue = m_device->GetCommandQueue(CommandListType::kGraphics);

    std::vector<uint32_t> index_data = { 0, 1, 2 };
    std::shared_ptr<Resource> index_buffer =
        m_device->CreateBuffer(MemoryType::kDefault, { .size = sizeof(index_data.front()) * index_data.size(),
                                                       .usage = BindFlag::kIndexBuffer | BindFlag::kCopyDest });
    index_buffer->SetName("index_buffer");
    std::vector<glm::vec3> vertex_data = {
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3(0.0, 0.5, 0.0),
        glm::vec3(0.5, -0.5, 0.0),
    };
    std::shared_ptr<Resource> vertex_buffer =
        m_device->CreateBuffer(MemoryType::kDefault, { .size = sizeof(vertex_data.front()) * vertex_data.size(),
                                                       .usage = BindFlag::kVertexBuffer | BindFlag::kCopyDest });
    vertex_buffer->SetName("vertex_buffer");

    std::shared_ptr<Resource> upload_buffer = m_device->CreateBuffer(
        MemoryType::kUpload,
        { .size = index_buffer->GetWidth() + vertex_buffer->GetWidth(), .usage = BindFlag::kCopySource });
    upload_buffer->SetName("upload_buffer");
    upload_buffer->UpdateUploadBuffer(0, index_data.data(), sizeof(index_data.front()) * index_data.size());
    upload_buffer->UpdateUploadBuffer(index_buffer->GetWidth(), vertex_data.data(),
                                      sizeof(vertex_data.front()) * vertex_data.size());

    std::shared_ptr<CommandList> upload_command_list = m_device->CreateCommandList(CommandListType::kGraphics);
    upload_command_list->CopyBuffer(upload_buffer, index_buffer, { { 0, 0, index_buffer->GetWidth() } });
    upload_command_list->CopyBuffer(upload_buffer, vertex_buffer,
                                    { { index_buffer->GetWidth(), 0, vertex_buffer->GetWidth() } });

    upload_command_list->ResourceBarrier(
        { { index_buffer, ResourceState::kCopyDest, ResourceState::kNonPixelShaderResource } });
    upload_command_list->ResourceBarrier(
        { { vertex_buffer, ResourceState::kCopyDest, ResourceState::kNonPixelShaderResource } });

    RaytracingGeometryDesc raytracing_geometry_desc = {
        { vertex_buffer, gli::format::FORMAT_RGB32_SFLOAT_PACK32, 3 },
        { index_buffer, gli::format::FORMAT_R32_UINT_PACK32, 3 },
        RaytracingGeometryFlags::kOpaque,
    };

    const uint32_t blas_count = 2;
    RaytracingASPrebuildInfo blas_prebuild_info =
        m_device->GetBLASPrebuildInfo({ raytracing_geometry_desc }, BuildAccelerationStructureFlags::kAllowCompaction);
    RaytracingASPrebuildInfo tlas_prebuild_info =
        m_device->GetTLASPrebuildInfo(blas_count, BuildAccelerationStructureFlags::kNone);
    uint64_t acceleration_structures_size =
        Align(blas_prebuild_info.acceleration_structure_size, kAccelerationStructureAlignment) +
        tlas_prebuild_info.acceleration_structure_size;
    m_acceleration_structures_memory = m_device->CreateBuffer(
        MemoryType::kDefault, { .size = acceleration_structures_size, .usage = BindFlag::kAccelerationStructure });
    m_acceleration_structures_memory->SetName("acceleration_structures_memory");

    std::shared_ptr<Resource> blas = m_device->CreateAccelerationStructure({
        .type = AccelerationStructureType::kBottomLevel,
        .buffer = m_acceleration_structures_memory,
        .buffer_offset = 0,
        .size = blas_prebuild_info.acceleration_structure_size,
    });

    std::shared_ptr<Resource> scratch = m_device->CreateBuffer(
        MemoryType::kDefault,
        { .size = std::max(blas_prebuild_info.build_scratch_data_size, tlas_prebuild_info.build_scratch_data_size),
          .usage = BindFlag::kRayTracing });
    scratch->SetName("scratch");

    std::shared_ptr<Resource> blas_compacted_size_buffer =
        m_device->CreateBuffer(MemoryType::kReadback, { .size = sizeof(uint64_t), .usage = BindFlag::kCopyDest });
    blas_compacted_size_buffer->SetName("blas_compacted_size_buffer");

    std::shared_ptr<QueryHeap> query_heap =
        m_device->CreateQueryHeap(QueryHeapType::kAccelerationStructureCompactedSize, 1);

    upload_command_list->BuildBottomLevelAS({}, blas, scratch, 0, { raytracing_geometry_desc },
                                            BuildAccelerationStructureFlags::kAllowCompaction);
    upload_command_list->UAVResourceBarrier(blas);
    upload_command_list->WriteAccelerationStructuresProperties({ blas }, query_heap, 0);
    upload_command_list->ResolveQueryData(query_heap, 0, 1, blas_compacted_size_buffer, 0);
    upload_command_list->Close();

    upload_command_queue->ExecuteCommandLists({ upload_command_list });
    upload_command_queue->Signal(m_fence, ++m_fence_value);
    m_fence->Wait(m_fence_value);

    uint64_t blas_compacted_size = *reinterpret_cast<uint64_t*>(blas_compacted_size_buffer->Map());
    blas_compacted_size_buffer->Unmap();

    upload_command_list->Reset();
    upload_command_list->CopyAccelerationStructure(blas, blas, CopyAccelerationStructureMode::kCompact);

    std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4x4>> geometry = {
        { blas, glm::transpose(glm::translate(glm::vec3(-0.5, 0.0, 0.0))) },
        { blas, glm::transpose(glm::translate(glm::vec3(0.5, 0.0, 0.0))) },
    };
    assert(geometry.size() == blas_count);
    std::vector<RaytracingGeometryInstance> instances;
    for (const auto& mesh : geometry) {
        RaytracingGeometryInstance& instance = instances.emplace_back();
        memcpy(&instance.transform, &mesh.second, sizeof(instance.transform));
        instance.instance_offset = static_cast<uint32_t>(instances.size() - 1);
        instance.instance_mask = 0xff;
        instance.acceleration_structure_handle = mesh.first->GetAccelerationStructureHandle();
    }

    m_tlas = m_device->CreateAccelerationStructure({
        .type = AccelerationStructureType::kTopLevel,
        .buffer = m_acceleration_structures_memory,
        .buffer_offset = Align(blas_compacted_size, kAccelerationStructureAlignment),
        .size = tlas_prebuild_info.acceleration_structure_size,
    });

    std::shared_ptr<Resource> instance_data = m_device->CreateBuffer(
        MemoryType::kUpload, { .size = instances.size() * sizeof(instances.back()), .usage = BindFlag::kRayTracing });
    instance_data->SetName("instance_data");
    instance_data->UpdateUploadBuffer(0, instances.data(), instances.size() * sizeof(instances.back()));
    upload_command_list->BuildTopLevelAS({}, m_tlas, scratch, 0, instance_data, 0, instances.size(),
                                         BuildAccelerationStructureFlags::kNone);
    upload_command_list->UAVResourceBarrier(m_tlas);

    upload_command_list->Close();

    upload_command_queue->ExecuteCommandLists({ upload_command_list });
    upload_command_queue->Signal(m_fence, ++m_fence_value);
    m_command_queue->Wait(m_fence, m_fence_value);

    ViewDesc tlas_view_desc = {
        .view_type = ViewType::kAccelerationStructure,
    };
    m_tlas_view = m_device->CreateView(m_tlas, tlas_view_desc);

    m_library = m_device->CompileShader({ "shaders/DxrTriangle/RayTracing.hlsl", "", ShaderType::kLibrary, "6_3" });
    m_library_hit =
        m_device->CompileShader({ "shaders/DxrTriangle/RayTracingHit.hlsl", "", ShaderType::kLibrary, "6_3" });
    m_library_callable =
        m_device->CompileShader({ "shaders/DxrTriangle/RayTracingCallable.hlsl", "", ShaderType::kLibrary, "6_3" });
    BindKey geometry_key = m_library->GetBindKey("geometry");
    BindKey result_texture_key = m_library->GetBindKey("result_texture");
    m_layout = m_device->CreateBindingSetLayout({ geometry_key, result_texture_key });

    std::vector<RayTracingShaderGroup> groups;
    groups.push_back({ RayTracingShaderGroupType::kGeneral, m_library->GetId("ray_gen") });
    groups.push_back({ RayTracingShaderGroupType::kGeneral, m_library->GetId("miss") });
    groups.push_back({ RayTracingShaderGroupType::kTrianglesHitGroup, 0, m_library_hit->GetId("closest_red") });
    groups.push_back({ RayTracingShaderGroupType::kTrianglesHitGroup, 0, m_library_hit->GetId("closest_green") });
    groups.push_back({ RayTracingShaderGroupType::kGeneral, m_library_callable->GetId("callable") });

    RayTracingPipelineDesc pipeline_desc = {
        .shaders = { m_library, m_library_hit, m_library_callable },
        .layout = m_layout,
        .groups = groups,
    };
    m_pipeline = m_device->CreateRayTracingPipeline(pipeline_desc);

    m_shader_table = m_device->CreateBuffer(
        MemoryType::kUpload,
        { .size = m_device->GetShaderTableAlignment() * groups.size(), .usage = BindFlag::kShaderTable });
    m_shader_table->SetName("shader_table");

    std::vector<uint8_t> shader_handles = m_pipeline->GetRayTracingShaderGroupHandles(0, groups.size());
    for (size_t i = 0; i < groups.size(); ++i) {
        m_shader_table->UpdateUploadBuffer(i * m_device->GetShaderTableAlignment(),
                                           shader_handles.data() + i * m_device->GetShaderGroupHandleSize(),
                                           m_device->GetShaderGroupHandleSize());
    }

    m_shader_tables = { .raygen = { m_shader_table, 0 * m_device->GetShaderTableAlignment(),
                                    m_device->GetShaderTableAlignment(), m_device->GetShaderTableAlignment() },
                        .miss = { m_shader_table, 1 * m_device->GetShaderTableAlignment(),
                                  m_device->GetShaderTableAlignment(), m_device->GetShaderTableAlignment() },
                        .hit = { m_shader_table, 2 * m_device->GetShaderTableAlignment(),
                                 2 * m_device->GetShaderTableAlignment(), m_device->GetShaderTableAlignment() },
                        .callable = { m_shader_table, 4 * m_device->GetShaderTableAlignment(),
                                      m_device->GetShaderTableAlignment(), m_device->GetShaderTableAlignment() } };
}

RayTracingTriangleRenderer::~RayTracingTriangleRenderer()
{
    WaitForIdle();
}

void RayTracingTriangleRenderer::Init(const AppSize& app_size, const NativeSurface& surface)
{
    m_swapchain =
        m_device->CreateSwapchain(surface, app_size.width(), app_size.height(), kFrameCount, m_settings.vsync);

    TextureDesc result_texture_desc = {
        .type = TextureType::k2D,
        .format = m_swapchain->GetFormat(),
        .width = app_size.width(),
        .height = app_size.height(),
        .depth_or_array_layers = 1,
        .mip_levels = 1,
        .sample_count = 1,
        .usage = BindFlag::kUnorderedAccess | BindFlag::kCopySource,
    };
    m_result_texture = m_device->CreateTexture(MemoryType::kDefault, result_texture_desc);
    m_result_texture->SetName("result_texture");

    ViewDesc result_texture_view_desc = {
        .view_type = ViewType::kRWTexture,
        .dimension = ViewDimension::kTexture2D,
    };
    m_result_texture_view = m_device->CreateView(m_result_texture, result_texture_view_desc);

    BindKey geometry_key = m_library->GetBindKey("geometry");
    BindKey result_texture_key = m_library->GetBindKey("result_texture");
    std::shared_ptr<BindingSet> m_binding_set = m_device->CreateBindingSet(m_layout);
    m_binding_set->WriteBindings({
        { geometry_key, m_tlas_view },
        { result_texture_key, m_result_texture_view },
    });

    for (uint32_t i = 0; i < kFrameCount; ++i) {
        std::shared_ptr<Resource> back_buffer = m_swapchain->GetBackBuffer(i);
        auto& command_list = m_command_lists[i];
        command_list = m_device->CreateCommandList(CommandListType::kGraphics);
        command_list->BindPipeline(m_pipeline);
        command_list->BindBindingSet(m_binding_set);
        command_list->DispatchRays(m_shader_tables, app_size.width(), app_size.height(), 1);
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kPresent, ResourceState::kCopyDest } });
        command_list->ResourceBarrier(
            { { m_result_texture, ResourceState::kUnorderedAccess, ResourceState::kCopySource } });
        command_list->CopyTexture(m_result_texture, back_buffer, { { app_size.width(), app_size.height(), 1 } });
        command_list->ResourceBarrier(
            { { m_result_texture, ResourceState::kCopySource, ResourceState::kUnorderedAccess } });
        command_list->ResourceBarrier({ { back_buffer, ResourceState::kCopyDest, ResourceState::kPresent } });
        command_list->Close();
    }
}

void RayTracingTriangleRenderer::Resize(const AppSize& app_size, const NativeSurface& surface)
{
    WaitForIdle();
    m_swapchain.reset();
    Init(app_size, surface);
}

void RayTracingTriangleRenderer::Render()
{
    uint32_t frame_index = m_swapchain->NextImage(m_fence, ++m_fence_value);
    m_command_queue->Wait(m_fence, m_fence_value);
    m_fence->Wait(m_fence_values[frame_index]);
    m_command_queue->ExecuteCommandLists({ m_command_lists[frame_index] });
    m_command_queue->Signal(m_fence, m_fence_values[frame_index] = ++m_fence_value);
    m_swapchain->Present(m_fence, m_fence_values[frame_index]);
}

std::string_view RayTracingTriangleRenderer::GetTitle() const
{
    return "RayTracingTriangle";
}

const std::string& RayTracingTriangleRenderer::GetGpuName() const
{
    return m_adapter->GetName();
}

const Settings& RayTracingTriangleRenderer::GetSettings() const
{
    return m_settings;
}

void RayTracingTriangleRenderer::WaitForIdle()
{
    m_command_queue->Signal(m_fence, ++m_fence_value);
    m_fence->Wait(m_fence_value);
}

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppLoop::Run(std::make_unique<RayTracingTriangleRenderer>(settings), argc, argv);
}

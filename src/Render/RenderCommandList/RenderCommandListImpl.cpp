#include "RenderCommandList/RenderCommandListImpl.h"
#include <Utilities/FormatHelper.h>

RenderCommandListImpl::RenderCommandListImpl(Device& device, ObjectCache& object_cache, CommandListType type)
    : m_device(device)
    , m_object_cache(object_cache)
{
    m_command_list = m_device.CreateCommandList(type);
}

const std::shared_ptr<CommandList>& RenderCommandListImpl::GetCommandList()
{
    return m_command_list;
}

void RenderCommandListImpl::Reset()
{
    m_lazy_barriers.clear();
    m_resource_state_tracker.clear();

    m_cmd_resources.clear();
    m_bound_resources.clear();
    m_bound_deferred_view.clear();
    m_binding_sets.clear();
    m_resource_lazy_view_descs.clear();
    m_shading_rate_image.reset();
    m_command_list->Reset();

    m_graphic_pipeline_desc = {};
    m_compute_pipeline_desc = {};
    m_ray_tracing_pipeline_desc = {};
}

void RenderCommandListImpl::Close()
{
    if (!kUseFakeClose)
        m_command_list->Close();
}

void RenderCommandListImpl::CopyBuffer(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_buffer,
                                       const std::vector<BufferCopyRegion>& regions)
{
    for (const auto& region : regions)
    {
        BufferBarrier(src_buffer, ResourceState::kCopySource);
        BufferBarrier(dst_buffer, ResourceState::kCopyDest);
    }
    m_command_list->CopyBuffer(src_buffer, dst_buffer, regions);
}

void RenderCommandListImpl::CopyBufferToTexture(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_texture,
                                                const std::vector<BufferToTextureCopyRegion>& regions)
{
    for (const auto& region : regions)
    {
        BufferBarrier(src_buffer, ResourceState::kCopySource);
        ImageBarrier(dst_texture, region.texture_mip_level, 1, region.texture_array_layer, 1, ResourceState::kCopyDest);
    }
    m_command_list->CopyBufferToTexture(src_buffer, dst_texture, regions);
}

void RenderCommandListImpl::CopyTexture(const std::shared_ptr<Resource>& src_texture, const std::shared_ptr<Resource>& dst_texture, const std::vector<TextureCopyRegion>& regions)
{
    for (const auto& region : regions)
    {
        ImageBarrier(src_texture, region.src_mip_level, 1, region.src_array_layer, 1, ResourceState::kCopySource);
        ImageBarrier(dst_texture, region.dst_mip_level, 1, region.dst_array_layer, 1, ResourceState::kCopyDest);
    }
    m_command_list->CopyTexture(src_texture, dst_texture, regions);
}

void RenderCommandListImpl::UpdateSubresource(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch, uint32_t depth_pitch)
{
    switch (resource->GetMemoryType())
    {
    case MemoryType::kUpload:
        return resource->UpdateUploadBuffer(0, data, resource->GetWidth());
    case MemoryType::kDefault:
        return UpdateDefaultSubresource(resource, subresource, data, row_pitch, depth_pitch);
    }
}

void RenderCommandListImpl::UpdateDefaultSubresource(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch, uint32_t depth_pitch)
{
    std::shared_ptr<Resource>& upload_resource = m_cmd_resources.emplace_back();

    switch (resource->GetResourceType())
    {
    case ResourceType::kBuffer:
    {
        size_t buffer_size = resource->GetWidth();
        if (!upload_resource)
        {
            upload_resource = m_device.CreateBuffer(BindFlag::kCopySource, buffer_size);
            upload_resource->CommitMemory(MemoryType::kUpload);
        }
        upload_resource->UpdateUploadBuffer(0, data, buffer_size);

        std::vector<BufferCopyRegion> regions;
        auto& region = regions.emplace_back();
        region.num_bytes = buffer_size;
        BufferBarrier(resource, ResourceState::kCopyDest);
        m_command_list->CopyBuffer(upload_resource, resource, regions);
        break;
    }
    case ResourceType::kTexture:
    {
        std::vector<BufferToTextureCopyRegion> regions;
        auto& region = regions.emplace_back();
        region.texture_mip_level = subresource % resource->GetLevelCount();
        region.texture_array_layer = subresource / resource->GetLevelCount();
        region.texture_extent.width = std::max<uint32_t>(1, resource->GetWidth() >> region.texture_mip_level);
        region.texture_extent.height = std::max<uint32_t>(1, resource->GetHeight() >> region.texture_mip_level);
        region.texture_extent.depth = 1;

        size_t num_bytes = 0, row_bytes = 0, num_rows = 0;
        GetFormatInfo(region.texture_extent.width, region.texture_extent.height, resource->GetFormat(), num_bytes, row_bytes, num_rows, m_device.GetTextureDataPitchAlignment());
        region.buffer_row_pitch = row_bytes;

        if (!upload_resource)
        {
            upload_resource = m_device.CreateBuffer(BindFlag::kCopySource, num_bytes);
            upload_resource->CommitMemory(MemoryType::kUpload);
        }
        upload_resource->UpdateUploadBufferWithTextureData(0, row_bytes, num_bytes, data, row_pitch, depth_pitch, num_rows, region.texture_extent.depth);

        ImageBarrier(resource, region.texture_mip_level, 1, region.texture_array_layer, 1, ResourceState::kCopyDest);
        m_command_list->CopyBufferToTexture(upload_resource, resource, regions);
        break;
    }
    }
}

void RenderCommandListImpl::SetViewport(float x, float y, float width, float height)
{
    m_viewport_width = width;
    m_viewport_height = height;
    m_command_list->SetViewport(x, y, width, height);
    m_command_list->SetScissorRect(x, y, width, height);
}

void RenderCommandListImpl::SetScissorRect(int32_t left, int32_t top, uint32_t right, uint32_t bottom)
{
    m_command_list->SetScissorRect(left, top, right, bottom);
}

void RenderCommandListImpl::IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format)
{
    BufferBarrier(resource, ResourceState::kIndexBuffer);
    m_command_list->IASetIndexBuffer(resource, format);
}

void RenderCommandListImpl::IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource)
{
    BufferBarrier(resource, ResourceState::kVertexAndConstantBuffer);
    m_command_list->IASetVertexBuffer(slot, resource);
}

void RenderCommandListImpl::RSSetShadingRateImage(const std::shared_ptr<View>& view)
{
    ViewBarrier(view, ResourceState::kShadingRateSource);
    m_shading_rate_image = view;
}

void RenderCommandListImpl::BeginEvent(const std::string& name)
{
    m_command_list->BeginEvent(name);
}

void RenderCommandListImpl::EndEvent()
{
    m_command_list->EndEvent();
}

void RenderCommandListImpl::Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
    Apply();
    m_command_list->Draw(vertex_count, instance_count, first_vertex, first_instance);
}

void RenderCommandListImpl::DrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance)
{
    Apply();
    m_command_list->DrawIndexed(index_count, instance_count, first_index, vertex_offset, first_instance);
}

void RenderCommandListImpl::DrawIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
    Apply();
    BufferBarrier(argument_buffer, ResourceState::kIndirectArgument);
    m_command_list->DrawIndirect(argument_buffer, argument_buffer_offset);
}

void RenderCommandListImpl::DrawIndexedIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
    Apply();
    BufferBarrier(argument_buffer, ResourceState::kIndirectArgument);
    m_command_list->DrawIndexedIndirect(argument_buffer, argument_buffer_offset);
}

void RenderCommandListImpl::DrawIndirectCount(
    const std::shared_ptr<Resource>& argument_buffer,
    uint64_t argument_buffer_offset,
    const std::shared_ptr<Resource>& count_buffer,
    uint64_t count_buffer_offset,
    uint32_t max_draw_count,
    uint32_t stride)
{
    Apply();
    BufferBarrier(argument_buffer, ResourceState::kIndirectArgument);
    m_command_list->DrawIndirectCount(
        argument_buffer,
        argument_buffer_offset,
        count_buffer,
        count_buffer_offset,
        max_draw_count,
        stride
    );
}

void RenderCommandListImpl::DrawIndexedIndirectCount(
    const std::shared_ptr<Resource>& argument_buffer,
    uint64_t argument_buffer_offset,
    const std::shared_ptr<Resource>& count_buffer,
    uint64_t count_buffer_offset,
    uint32_t max_draw_count,
    uint32_t stride)
{
    Apply();
    BufferBarrier(argument_buffer, ResourceState::kIndirectArgument);
    m_command_list->DrawIndexedIndirectCount(
        argument_buffer,
        argument_buffer_offset,
        count_buffer,
        count_buffer_offset,
        max_draw_count,
        stride
    );
}

void RenderCommandListImpl::Dispatch(uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z)
{
    Apply();
    m_command_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z);
}

void RenderCommandListImpl::DispatchIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
    Apply();
    BufferBarrier(argument_buffer, ResourceState::kIndirectArgument);
    m_command_list->DispatchIndirect(argument_buffer, argument_buffer_offset);
}

void RenderCommandListImpl::DispatchMesh(uint32_t thread_group_count_x)
{
    Apply();
    m_command_list->DispatchMesh(thread_group_count_x);
}

void RenderCommandListImpl::DispatchRays(uint32_t width, uint32_t height, uint32_t depth)
{
    Apply();
    m_command_list->DispatchRays(m_shader_tables, width, height, depth);
}

void RenderCommandListImpl::BufferBarrier(const std::shared_ptr<Resource>& resource, ResourceState state)
{
    if (!resource)
        return;
    LazyResourceBarrierDesc barrier = {};
    barrier.resource = resource;
    barrier.state = state;
    LazyResourceBarrier({ barrier });
}

void RenderCommandListImpl::ViewBarrier(const std::shared_ptr<View>& view, ResourceState state)
{
    if (!view || !view->GetResource())
        return;
    ImageBarrier(view->GetResource(), view->GetBaseMipLevel(), view->GetLevelCount(), view->GetBaseArrayLayer(), view->GetLayerCount(), state);
}

void RenderCommandListImpl::ImageBarrier(const std::shared_ptr<Resource>& resource, uint32_t base_mip_level, uint32_t level_count, uint32_t base_array_layer, uint32_t layer_count, ResourceState state)
{
    if (!resource)
        return;
    LazyResourceBarrierDesc barrier = {};
    barrier.resource = resource;
    barrier.base_mip_level = base_mip_level;
    barrier.level_count = level_count;
    barrier.base_array_layer = base_array_layer;
    barrier.layer_count = layer_count;
    barrier.state = state;
    LazyResourceBarrier({ barrier });
}

void RenderCommandListImpl::BuildBottomLevelAS(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, const std::vector<RaytracingGeometryDesc>& descs, BuildAccelerationStructureFlags flags)
{
    for (const auto& desc : descs)
    {
        if (desc.vertex.res)
            BufferBarrier(desc.vertex.res, ResourceState::kNonPixelShaderResource);
        if (desc.index.res)
            BufferBarrier(desc.index.res, ResourceState::kNonPixelShaderResource);
    }

    RaytracingASPrebuildInfo prebuild_info = m_device.GetBLASPrebuildInfo(descs, flags);
    auto scratch = m_device.CreateBuffer(BindFlag::kRayTracing, src ? prebuild_info.update_scratch_data_size : prebuild_info.build_scratch_data_size);
    scratch->CommitMemory(MemoryType::kDefault);
    m_command_list->BuildBottomLevelAS(src, dst, scratch, 0, descs, flags);
    m_command_list->UAVResourceBarrier(dst);
    m_cmd_resources.emplace_back(scratch);
}

void RenderCommandListImpl::BuildTopLevelAS(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, const std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4>>& geometry, BuildAccelerationStructureFlags flags)
{
    std::vector<RaytracingGeometryInstance> instances;
    for (const auto& mesh : geometry)
    {
        RaytracingGeometryInstance& instance = instances.emplace_back();
        memcpy(&instance.transform, &mesh.second, sizeof(instance.transform));
        instance.instance_id = static_cast<uint32_t>(instances.size() - 1);
        instance.instance_mask = 0xff;
        instance.acceleration_structure_handle = mesh.first->GetAccelerationStructureHandle();
    }

    auto instance_data = m_device.CreateBuffer(BindFlag::kRayTracing, instances.size() * sizeof(instances.back()));
    instance_data->CommitMemory(MemoryType::kUpload);
    instance_data->UpdateUploadBuffer(0, instances.data(), instances.size() * sizeof(instances.back()));

    RaytracingASPrebuildInfo prebuild_info = m_device.GetTLASPrebuildInfo(instances.size(), flags);
    auto scratch = m_device.CreateBuffer(BindFlag::kRayTracing, src ? prebuild_info.update_scratch_data_size : prebuild_info.build_scratch_data_size);
    scratch->CommitMemory(MemoryType::kDefault);

    m_command_list->BuildTopLevelAS(src, dst, scratch, 0, instance_data, 0, geometry.size(), flags);
    m_command_list->UAVResourceBarrier(dst);

    m_cmd_resources.emplace_back(scratch);
    m_cmd_resources.emplace_back(instance_data);
}

void RenderCommandListImpl::CopyAccelerationStructure(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, CopyAccelerationStructureMode mode)
{
    m_command_list->CopyAccelerationStructure(src, dst, mode);
}

void RenderCommandListImpl::UseProgram(const std::shared_ptr<Program>& program)
{
    m_program = program;
    m_layout = m_object_cache.GetBindingSetLayout(m_program->GetBindings());

    if (m_program->HasShader(ShaderType::kCompute))
    {
        m_pipeline_type = PipelineType::kCompute;
        m_compute_pipeline_desc.program = m_program;
        m_compute_pipeline_desc.layout = m_layout;
    }
    else if (m_program->HasShader(ShaderType::kLibrary))
    {
        m_pipeline_type = PipelineType::kRayTracing;
        m_ray_tracing_pipeline_desc.program = m_program;
        m_ray_tracing_pipeline_desc.layout = m_layout;

        std::vector<RayTracingShaderGroup>& groups = m_ray_tracing_pipeline_desc.groups;
        m_shader_tables = {};
        m_shader_tables.raygen.stride = m_device.GetShaderTableAlignment();
        m_shader_tables.miss.stride = m_device.GetShaderTableAlignment();
        m_shader_tables.callable.stride = m_device.GetShaderTableAlignment();
        m_shader_tables.hit.stride = m_device.GetShaderTableAlignment();
        for (const auto& shader : m_program->GetShaders())
        {
            for (const auto& entry_point : shader->GetReflection()->GetEntryPoints())
            {
                switch (entry_point.kind)
                {
                case ShaderKind::kRayGeneration:
                    m_shader_tables.raygen.size += m_device.GetShaderTableAlignment();
                    break;
                case ShaderKind::kMiss:
                    m_shader_tables.miss.size += m_device.GetShaderTableAlignment();
                    break;
                case ShaderKind::kCallable:
                    m_shader_tables.callable.size += m_device.GetShaderTableAlignment();
                    break;
                case ShaderKind::kAnyHit:
                case ShaderKind::kClosestHit:
                case ShaderKind::kIntersection:
                    m_shader_tables.hit.size += m_device.GetShaderTableAlignment();
                    break;
                }

                switch (entry_point.kind)
                {
                case ShaderKind::kRayGeneration:
                case ShaderKind::kMiss:
                case ShaderKind::kCallable:
                    groups.push_back({ RayTracingShaderGroupType::kGeneral, shader->GetId(entry_point.name) });
                    break;
                case ShaderKind::kClosestHit:
                    groups.push_back({ RayTracingShaderGroupType::kTrianglesHitGroup, 0, shader->GetId(entry_point.name) });
                    break;
                case ShaderKind::kAnyHit:
                    groups.push_back({ RayTracingShaderGroupType::kTrianglesHitGroup, 0, 0, shader->GetId(entry_point.name) });
                    break;
                case ShaderKind::kIntersection:
                    groups.push_back({ RayTracingShaderGroupType::kProceduralHitGroup, 0, 0, 0, shader->GetId(entry_point.name) });
                    break;
                }
            }
        }
    }
    else
    {
        m_pipeline_type = PipelineType::kGraphics;
        m_graphic_pipeline_desc.program = m_program;
        m_graphic_pipeline_desc.layout = m_layout;
        if (m_program->HasShader(ShaderType::kVertex))
            m_graphic_pipeline_desc.input = m_program->GetShader(ShaderType::kVertex)->GetInputLayouts();
    }
    m_bound_resources.clear();
    m_bound_deferred_view.clear();
}

void RenderCommandListImpl::CreateShaderTable(std::shared_ptr<Pipeline> pipeline)
{
    decltype(auto) shader_handles = pipeline->GetRayTracingShaderGroupHandles(0, m_ray_tracing_pipeline_desc.groups.size());

    uint64_t table_size = m_shader_tables.raygen.size + m_shader_tables.miss.size + m_shader_tables.callable.size + m_shader_tables.hit.size;
    m_shader_table = m_device.CreateBuffer(BindFlag::kShaderTable, table_size);
    m_shader_table->CommitMemory(MemoryType::kUpload);

    m_shader_tables.raygen.resource = m_shader_table;
    m_shader_tables.miss.resource = m_shader_table;
    m_shader_tables.callable.resource = m_shader_table;
    m_shader_tables.hit.resource = m_shader_table;

    m_shader_tables.raygen.offset = 0;
    m_shader_tables.miss.offset = m_shader_tables.raygen.offset + m_shader_tables.raygen.size;
    m_shader_tables.callable.offset = m_shader_tables.miss.offset + m_shader_tables.miss.size;
    m_shader_tables.hit.offset = m_shader_tables.callable.offset + m_shader_tables.callable.size;

    size_t group_id = 0;
    uint64_t raygen_offset = 0;
    uint64_t miss_offset = 0;
    uint64_t callable_offset = 0;
    uint64_t hit_offset = 0;
    for (const auto& shader : m_program->GetShaders())
    {
        for (const auto& entry_point : shader->GetReflection()->GetEntryPoints())
        {
            switch (entry_point.kind)
            {
            case ShaderKind::kRayGeneration:
                m_shader_table->UpdateUploadBuffer(m_shader_tables.raygen.offset + raygen_offset, shader_handles.data() + m_device.GetShaderGroupHandleSize() * group_id, m_device.GetShaderGroupHandleSize());
                raygen_offset += m_device.GetShaderTableAlignment();
                break;
            case ShaderKind::kMiss:
                m_shader_table->UpdateUploadBuffer(m_shader_tables.miss.offset + miss_offset, shader_handles.data() + m_device.GetShaderGroupHandleSize() * group_id, m_device.GetShaderGroupHandleSize());
                miss_offset += m_device.GetShaderTableAlignment();
                break;
            case ShaderKind::kCallable:
                m_shader_table->UpdateUploadBuffer(m_shader_tables.callable.offset + callable_offset, shader_handles.data() + m_device.GetShaderGroupHandleSize() * group_id, m_device.GetShaderGroupHandleSize());
                callable_offset += m_device.GetShaderTableAlignment();
                break;
            case ShaderKind::kAnyHit:
            case ShaderKind::kClosestHit:
            case ShaderKind::kIntersection:
                m_shader_table->UpdateUploadBuffer(m_shader_tables.hit.offset + hit_offset, shader_handles.data() + m_device.GetShaderGroupHandleSize() * group_id, m_device.GetShaderGroupHandleSize());
                hit_offset += m_device.GetShaderTableAlignment();
                break;
            }
            ++group_id;
        }
    }
}

void RenderCommandListImpl::Apply()
{
    ApplyPipeline();
    if (m_pipeline_type == PipelineType::kRayTracing)
    {
        CreateShaderTable(m_pipeline);
        m_cmd_resources.emplace_back(m_shader_table);
    }
    ApplyBindingSet();
}

void RenderCommandListImpl::ApplyPipeline()
{
    switch (m_pipeline_type)
    {
    case PipelineType::kGraphics:
        m_pipeline = m_object_cache.GetPipeline(m_graphic_pipeline_desc);
        break;
    case PipelineType::kCompute:
        m_pipeline = m_object_cache.GetPipeline(m_compute_pipeline_desc);
        break;
    case PipelineType::kRayTracing:
        m_pipeline = m_object_cache.GetPipeline(m_ray_tracing_pipeline_desc);
        break;
    }
    m_command_list->BindPipeline(m_pipeline);
}

void RenderCommandListImpl::ApplyBindingSet()
{
    for (auto& x : m_bound_deferred_view)
    {
        auto view = x.second->GetView(*this);
        m_resource_lazy_view_descs.emplace_back(view);
        Attach(x.first, m_object_cache.GetView(m_program, x.first, view->resource, view->view_desc));
    }

    std::vector<BindingDesc> descs;
    descs.reserve(m_bound_resources.size());
    for (auto& x : m_bound_resources)
    {
        BindingDesc& desc = descs.emplace_back();
        desc.bind_key = x.first;
        desc.view = x.second;
    }

    std::shared_ptr<BindingSet> binding_set = m_object_cache.GetBindingSet(m_layout, descs);
    m_command_list->BindBindingSet(binding_set);
    m_binding_sets.emplace_back(binding_set);
}

void RenderCommandListImpl::SetBinding(const BindKey& bind_key, const std::shared_ptr<View>& view)
{
    auto it = m_bound_resources.find(bind_key);
    if (it == m_bound_resources.end())
        m_bound_resources.emplace(bind_key, view);
    else
        it->second = view;
}

void RenderCommandListImpl::BeginRenderPass(const RenderPassBeginDesc& desc)
{
    RenderPassDesc render_pass_desc = {};
    ClearDesc clear_desc = {};
    render_pass_desc.colors.reserve(desc.colors.size());
    for (const auto& color_desc : desc.colors)
    {
        auto& color = render_pass_desc.colors.emplace_back();
        clear_desc.colors.emplace_back() = color_desc.clear_color;
        if (!color_desc.texture)
            continue;
        color.format = color_desc.texture->GetFormat();
        color.load_op = color_desc.load_op;
        color.store_op = color_desc.store_op;
        render_pass_desc.sample_count = color_desc.texture->GetSampleCount();
    }

    if (desc.depth_stencil.texture)
    {
        render_pass_desc.depth_stencil.format = desc.depth_stencil.texture->GetFormat();
        render_pass_desc.depth_stencil.depth_load_op = desc.depth_stencil.depth_load_op;
        render_pass_desc.depth_stencil.depth_store_op = desc.depth_stencil.depth_store_op;
        render_pass_desc.depth_stencil.stencil_load_op = desc.depth_stencil.stencil_load_op;
        render_pass_desc.depth_stencil.stencil_store_op = desc.depth_stencil.stencil_store_op;
        render_pass_desc.sample_count = desc.depth_stencil.texture->GetSampleCount();
        clear_desc.depth = desc.depth_stencil.clear_depth;
        clear_desc.stencil = desc.depth_stencil.clear_stencil;
    }

    if (m_shading_rate_image)
    {
        render_pass_desc.shading_rate_format = m_shading_rate_image->GetResource()->GetFormat();
    }
    else
    {
        render_pass_desc.shading_rate_format = gli::format::FORMAT_UNDEFINED;
    }

    std::vector<std::shared_ptr<View>> rtvs;
    for (size_t i = 0; i < desc.colors.size(); ++i)
    {
        auto& view = rtvs.emplace_back();
        if (!desc.colors[i].texture)
        {
            continue;
        }

        BindKey bind_key = { ShaderType::kPixel, ViewType::kRenderTarget, i, 0 };
        view = m_object_cache.GetView(m_program, bind_key, desc.colors[i].texture, desc.colors[i].view_desc);
        ViewBarrier(view, ResourceState::kRenderTarget);
    }

    std::shared_ptr<View> dsv;
    if (desc.depth_stencil.texture)
    {
        BindKey bind_key = { ShaderType::kPixel, ViewType::kDepthStencil, 0, 0 };
        dsv = m_object_cache.GetView(m_program, bind_key, desc.depth_stencil.texture, desc.depth_stencil.view_desc);
        ViewBarrier(dsv, ResourceState::kDepthStencilWrite);
    }

    std::shared_ptr<RenderPass> render_pass = m_object_cache.GetRenderPass(render_pass_desc);
    m_graphic_pipeline_desc.render_pass = render_pass;

    FramebufferDesc framebuffer_desc = {};
    framebuffer_desc.render_pass = render_pass;
    framebuffer_desc.width = m_viewport_width;
    framebuffer_desc.height = m_viewport_height;
    framebuffer_desc.colors = rtvs;
    framebuffer_desc.depth_stencil = dsv;
    framebuffer_desc.shading_rate_image = m_shading_rate_image;
    std::shared_ptr<Framebuffer> framebuffer = m_object_cache.GetFramebuffer(framebuffer_desc);

    std::array<ShadingRateCombiner, 2> combiners = {
        ShadingRateCombiner::kPassthrough, ShadingRateCombiner::kPassthrough
    };
    if (m_shading_rate_image)
    {
        combiners[1] = ShadingRateCombiner::kOverride;
    }
    if (m_shading_rate_combiner != combiners[1])
    {
        m_command_list->RSSetShadingRate(ShadingRate::k1x1, combiners);
        m_shading_rate_combiner = combiners[1];
    }

    m_command_list->BeginRenderPass(render_pass, framebuffer, clear_desc);
}

void RenderCommandListImpl::EndRenderPass()
{
    m_command_list->EndRenderPass();
}

void RenderCommandListImpl::Attach(const BindKey& bind_key, const std::shared_ptr<DeferredView>& view)
{
    m_bound_deferred_view[bind_key] = view;
}

void RenderCommandListImpl::Attach(const BindKey& bind_key, const std::shared_ptr<Resource>& resource, const LazyViewDesc& view_desc)
{
    Attach(bind_key, m_object_cache.GetView(m_program, bind_key, resource, view_desc));
}

void RenderCommandListImpl::Attach(const BindKey& bind_key, const std::shared_ptr<View>& view)
{
    SetBinding(bind_key, view);
    switch (bind_key.view_type)
    {
    case ViewType::kTexture:
    case ViewType::kBuffer:
    case ViewType::kStructuredBuffer:
        OnAttachSRV(bind_key, view);
        break;
    case ViewType::kRWTexture:
    case ViewType::kRWBuffer:
    case ViewType::kRWStructuredBuffer:
        OnAttachUAV(bind_key, view);
        break;
    case ViewType::kRenderTarget:
    case ViewType::kDepthStencil:
        assert(false);
        break;
    }
}

void RenderCommandListImpl::SetRasterizeState(const RasterizerDesc& desc)
{
    m_graphic_pipeline_desc.rasterizer_desc = desc;
}

void RenderCommandListImpl::SetBlendState(const BlendDesc& desc)
{
    m_graphic_pipeline_desc.blend_desc = desc;
}

void RenderCommandListImpl::SetDepthStencilState(const DepthStencilDesc& desc)
{
    m_graphic_pipeline_desc.depth_stencil_desc = desc;
}

void RenderCommandListImpl::OnAttachSRV(const BindKey& bind_key, const std::shared_ptr<View>& view)
{
    if (bind_key.shader_type == ShaderType::kPixel)
        ViewBarrier(view, ResourceState::kPixelShaderResource);
    else
        ViewBarrier(view, ResourceState::kNonPixelShaderResource);
}

void RenderCommandListImpl::OnAttachUAV(const BindKey& bind_key, const std::shared_ptr<View>& view)
{
    ViewBarrier(view, ResourceState::kUnorderedAccess);
}

ResourceStateTracker& RenderCommandListImpl::GetResourceStateTracker(const std::shared_ptr<Resource>& resource)
{
    auto it = m_resource_state_tracker.find(resource);
    if (it == m_resource_state_tracker.end())
        it = m_resource_state_tracker.emplace(resource, *resource).first;
    return it->second;
}

const std::map<std::shared_ptr<Resource>, ResourceStateTracker>& RenderCommandListImpl::GetResourceStateTrackers() const
{
    return m_resource_state_tracker;
}

const std::vector<ResourceBarrierDesc>& RenderCommandListImpl::GetLazyBarriers() const
{
    return m_lazy_barriers;
}

void RenderCommandListImpl::LazyResourceBarrier(const std::vector<LazyResourceBarrierDesc>& barriers)
{
    std::vector<ResourceBarrierDesc> manual_barriers;
    for (const auto& barrier : barriers)
    {
        auto& state_tracker = GetResourceStateTracker(barrier.resource);
        if (state_tracker.HasResourceState() && barrier.base_mip_level == 0 && barrier.level_count == barrier.resource->GetLevelCount() &&
            barrier.base_array_layer == 0 && barrier.layer_count == barrier.resource->GetLayerCount())
        {
            ResourceBarrierDesc manual_barrier = {};
            manual_barrier.resource = barrier.resource;
            manual_barrier.level_count = barrier.level_count;
            manual_barrier.layer_count = barrier.layer_count;
            manual_barrier.state_before = state_tracker.GetResourceState();
            manual_barrier.state_after = barrier.state;
            state_tracker.SetResourceState(manual_barrier.state_after);
            if (manual_barrier.state_before != ResourceState::kUnknown)
                manual_barriers.emplace_back(manual_barrier);
            else
                m_lazy_barriers.emplace_back(manual_barrier);
        }
        else
        {
            for (uint32_t i = 0; i < barrier.level_count; ++i)
            {
                for (uint32_t j = 0; j < barrier.layer_count; ++j)
                {
                    ResourceBarrierDesc manual_barrier = {};
                    manual_barrier.resource = barrier.resource;
                    manual_barrier.base_mip_level = barrier.base_mip_level + i;
                    manual_barrier.level_count = 1;
                    manual_barrier.base_array_layer = barrier.base_array_layer + j;
                    manual_barrier.layer_count = 1;
                    manual_barrier.state_before = state_tracker.GetSubresourceState(manual_barrier.base_mip_level, manual_barrier.base_array_layer);
                    manual_barrier.state_after = barrier.state;
                    state_tracker.SetSubresourceState(manual_barrier.base_mip_level, manual_barrier.base_array_layer, manual_barrier.state_after);
                    if (manual_barrier.state_before != ResourceState::kUnknown)
                        manual_barriers.emplace_back(manual_barrier);
                    else
                        m_lazy_barriers.emplace_back(manual_barrier);
                }
            }
        }
    }
    if (!manual_barriers.empty())
        m_command_list->ResourceBarrier(manual_barriers);
}

uint64_t& RenderCommandListImpl::GetFenceValue()
{
    return m_fence_value;
}

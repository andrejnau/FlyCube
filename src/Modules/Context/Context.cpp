#include "Context/Context.h"
#include <Utilities/FormatHelper.h>

Context::Context(const Settings& settings, GLFWwindow* window)
    : m_window(window)
{
    glfwGetWindowSize(window, &m_width, &m_height);

    m_instance = CreateInstance(settings.api_type);
    m_adapter = std::move(m_instance->EnumerateAdapters()[settings.required_gpu_index]);
    m_device = m_adapter->CreateDevice();
    m_swapchain = m_device->CreateSwapchain(window, m_width, m_height, FrameCount, settings.vsync);
    for (uint32_t i = 0; i < FrameCount; ++i)
    {
        m_command_lists.emplace_back(m_device->CreateCommandList());
    }
    m_fence = m_device->CreateFence();
    m_command_list = m_command_lists.front();
    m_command_list->Open();
    m_image_available_semaphore = m_device->CreateGPUSemaphore();
    m_rendering_finished_semaphore = m_device->CreateGPUSemaphore();
}

std::shared_ptr<ProgramApi> Context::CreateProgram()
{
    return m_created_program.emplace_back(std::make_shared<ProgramApi>(*this));
}

std::shared_ptr<Resource> Context::CreateTexture(uint32_t bind_flag, gli::format format, uint32_t msaa_count, int width, int height, int depth, int mip_levels)
{
    return m_device->CreateTexture(bind_flag | BindFlag::kCopyDest, format, msaa_count, width, height, depth, mip_levels);
}

std::shared_ptr<Resource> Context::CreateBuffer(uint32_t bind_flag, uint32_t buffer_size)
{
    return m_device->CreateBuffer(bind_flag | BindFlag::kCopyDest, buffer_size, MemoryType::kDefault);
}

std::shared_ptr<Resource> Context::CreateSampler(const SamplerDesc& desc)
{
    return m_device->CreateSampler(desc);
}

std::shared_ptr<Resource> Context::CreateBottomLevelAS(const BufferDesc& vertex, const BufferDesc& index)
{
    if (m_is_open_render_pass)
    {
        m_command_list->EndRenderPass();
        m_is_open_render_pass = false;
    }
    if (vertex.res)
        ResourceBarrier(vertex.res, ResourceState::kNonPixelShaderResource);
    if (index.res)
        ResourceBarrier(index.res, ResourceState::kNonPixelShaderResource);
    return m_device->CreateBottomLevelAS(m_command_list, vertex, index);
}

std::shared_ptr<Resource> Context::CreateTopLevelAS(const std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4>>& geometry)
{
    if (m_is_open_render_pass)
    {
        m_command_list->EndRenderPass();
        m_is_open_render_pass = false;
    }
    std::vector<GeometryInstance> instances;
    for (const auto& mesh : geometry)
    {
        GeometryInstance& instance = instances.emplace_back();
        memcpy(&instance.transform, &mesh.second, sizeof(instance.transform));
        instance.instance_id = static_cast<uint32_t>(instances.size() - 1);
        instance.instance_mask = 0xff;
        instance.acceleration_structure_handle = mesh.first->GetAccelerationStructureHandle();
    }

    auto instance_data = m_device->CreateBuffer(BindFlag::kRayTracing, instances.size() * sizeof(instances.back()), MemoryType::kUpload);
    instance_data->UpdateUploadData(instances.data(), 0, instances.size() * sizeof(instances.back()));

    return m_device->CreateTopLevelAS(m_command_list, instance_data, geometry.size());
}

void Context::UpdateSubresource(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch , uint32_t depth_pitch)
{
    switch (resource->GetMemoryType())
    {
    case MemoryType::kUpload:
       return resource->UpdateUploadData(data, 0, resource->GetWidth());
    case MemoryType::kDefault:
        return UpdateSubresourceDefault(resource, subresource, data, row_pitch, depth_pitch);
    }
}

void Context::UpdateSubresourceDefault(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch, uint32_t depth_pitch)
{
    if (m_is_open_render_pass)
    {
        m_command_list->EndRenderPass();
        m_is_open_render_pass = false;
    }

    std::shared_ptr<Resource>& upload_resource = resource->GetPrivateResource(subresource);

    switch (resource->GetResourceType())
    {
    case ResourceType::kBuffer:
    {
        size_t buffer_size = resource->GetWidth();
        if (!upload_resource)
            upload_resource = m_device->CreateBuffer(BindFlag::kCopySource, buffer_size, MemoryType::kUpload);
        upload_resource->UpdateUploadData(data, 0, buffer_size);

        std::vector<BufferCopyRegion> regions;
        auto& region = regions.emplace_back();
        region.num_bytes = buffer_size;
        ResourceBarrier(resource, ResourceState::kCopyDest);
        m_command_list->CopyBuffer(upload_resource, resource, regions);
        break;
    }
    case ResourceType::kTexture:
    {
        std::vector<BufferToTextureCopyRegion> regions;
        auto& region = regions.emplace_back();
        region.texture_subresource = subresource;
        region.texture_extent.width = std::max<uint32_t>(1, resource->GetWidth() >> subresource);
        region.texture_extent.height = std::max<uint32_t>(1, resource->GetHeight() >> subresource);
        region.texture_extent.depth = 1;

        size_t num_bytes = 0, row_bytes = 0, num_rows = 0;
        GetFormatInfo(region.texture_extent.width, region.texture_extent.height, resource->GetFormat(), num_bytes, row_bytes, num_rows, m_device->GetTextureDataPitchAlignment());
        region.buffer_row_pitch = row_bytes;

        if (!upload_resource)
            upload_resource = m_device->CreateBuffer(BindFlag::kCopySource, num_bytes, MemoryType::kUpload);
        upload_resource->UpdateSubresource(0, row_bytes, num_bytes, data, row_pitch, depth_pitch, num_rows, region.texture_extent.depth);

        ResourceBarrier(resource, ResourceState::kCopyDest);
        m_command_list->CopyBufferToTexture(upload_resource, resource, regions);
        break;
    }
    }
}

void Context::SetViewport(float width, float height)
{
    m_command_list->SetViewport(width, height);
    m_viewport_width = width;
    m_viewport_height = height;
    if (m_current_program)
        m_current_program->OnSetViewport(m_viewport_width, m_viewport_height);
}

void Context::SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
}

void Context::IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format)
{
    ResourceBarrier(resource, ResourceState::kIndexBuffer);
    m_command_list->IASetIndexBuffer(resource, format);
}

void Context::IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource)
{
    ResourceBarrier(resource, ResourceState::kVertexAndConstantBuffer);
    m_command_list->IASetVertexBuffer(slot, resource);
}

void Context::UseProgram(ProgramApi& program)
{
    if (m_current_program != &program)
    {
        if (m_current_program)
            m_current_program->ProgramDetach();
        m_current_program = &program;
        if (m_is_open_render_pass)
        {
            m_command_list->EndRenderPass();
            m_is_open_render_pass = false;
        }
    }
    if (m_current_program)
        m_current_program->OnSetViewport(m_viewport_width, m_viewport_height);
}

void Context::BeginEvent(const std::string& name)
{
    m_command_list->BeginEvent(name);
}

void Context::EndEvent()
{
    m_command_list->EndEvent();
}

void Context::DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation)
{
    if (m_current_program)
        m_current_program->ApplyBindings();
    m_command_list->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}

void Context::Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
{
    if (m_current_program)
        m_current_program->ApplyBindings();
    m_command_list->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void Context::DispatchRays(uint32_t width, uint32_t height, uint32_t depth)
{
    if (m_current_program)
        m_current_program->ApplyBindings();
    m_command_list->DispatchRays(width, height, depth);
}

bool Context::IsDxrSupported() const
{
    return m_device->IsDxrSupported();
}

std::shared_ptr<Resource> Context::GetBackBuffer()
{
    return m_swapchain->GetBackBuffer(m_frame_index);
}

void Context::Present()
{
    if (m_is_open_render_pass)
    {
        m_command_list->EndRenderPass();
        m_is_open_render_pass = false;
    }
    ResourceBarrier(GetBackBuffer(), ResourceState::kPresent);
    m_command_list->Close();
    m_fence->WaitAndReset();
    m_swapchain->NextImage(m_image_available_semaphore);
    m_device->Wait(m_image_available_semaphore);
    m_device->ExecuteCommandLists({ m_command_list }, m_fence);
    m_device->Signal(m_rendering_finished_semaphore);
    m_swapchain->Present(m_rendering_finished_semaphore);

    ++m_frame_index;
    m_frame_index %= FrameCount;
    m_command_list = m_command_lists[m_frame_index];
    m_command_list->Open();

    for (auto& x : m_created_program)
        x->OnPresent();
}

void Context::ResourceBarrier(const std::shared_ptr<Resource>& resource, ResourceState state)
{
    if (!resource)
        return;
    ResourceBarrierDesc barrier = {};
    barrier.resource = resource;
    barrier.state_before = resource->GetResourceState();
    barrier.state_after = state;
    barrier.level_count = resource->GetMipLevels();
    barrier.layer_count = resource->GetDepthOrArraySize();
    m_command_list->ResourceBarrier({ barrier });
    resource->SetResourceState(state);
}

void Context::OnResize(int width, int height)
{
    m_width = width;
    m_height = height;
    ResizeBackBuffer(m_width, m_height);
}

size_t Context::GetFrameIndex() const
{
    return m_frame_index;
}

GLFWwindow* Context::GetWindow()
{
    return m_window;
}

void Context::ResizeBackBuffer(int width, int height)
{
}

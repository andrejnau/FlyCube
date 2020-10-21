#include "RenderDevice/RenderDeviceImpl.h"
#include <Utilities/FormatHelper.h>
#include <RenderCommandList/RenderCommandListImpl.h>
#include <Resource/ResourceBase.h>

RenderDeviceImpl::RenderDeviceImpl(const Settings& settings, GLFWwindow* window)
    : m_window(window)
    , m_vsync(settings.vsync)
    , m_frame_count(settings.frame_count)
{
    m_instance = CreateInstance(settings.api_type);
    m_adapter = std::move(m_instance->EnumerateAdapters()[settings.required_gpu_index]);
    m_device = m_adapter->CreateDevice();
    m_command_queue = m_device->GetCommandQueue(CommandListType::kGraphics);

    glfwGetWindowSize(window, &m_width, &m_height);
    m_swapchain = m_device->CreateSwapchain(window, m_width, m_height, m_frame_count, settings.vsync);
    m_fence = m_device->CreateFence(m_fence_value);
    for (uint32_t i = 0; i < m_frame_count; ++i)
    {
        m_swapchain_command_lists.emplace_back(m_device->CreateCommandList());
        m_swapchain_fence_values.emplace_back(0);
    }
}

RenderDeviceImpl::~RenderDeviceImpl()
{
    m_command_list_pool.clear();
    WaitForIdle();
}

gli::format RenderDeviceImpl::GetFormat() const
{
    return m_swapchain->GetFormat();
}

std::shared_ptr<Resource> RenderDeviceImpl::GetBackBuffer(uint32_t buffer)
{
    return m_swapchain->GetBackBuffer(buffer);
}

std::shared_ptr<RenderCommandList> RenderDeviceImpl::CreateRenderCommandList(CommandListType type)
{
    return std::make_shared<RenderCommandListImpl>(*m_device, CommandListType::kGraphics);
}

std::shared_ptr<Resource> RenderDeviceImpl::CreateTexture(uint32_t bind_flag, gli::format format, uint32_t sample_count, int width, int height, int depth, int mip_levels)
{
    return m_device->CreateTexture(bind_flag, format, sample_count, width, height, depth, mip_levels);
}

std::shared_ptr<Resource> RenderDeviceImpl::CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, MemoryType memory_type)
{
    return m_device->CreateBuffer(bind_flag, buffer_size, memory_type);
}

std::shared_ptr<Resource> RenderDeviceImpl::CreateSampler(const SamplerDesc& desc)
{
    return m_device->CreateSampler(desc);
}

std::shared_ptr<Resource> RenderDeviceImpl::CreateBottomLevelAS(const std::vector<RaytracingGeometryDesc>& descs, BuildAccelerationStructureFlags flags)
{
    return m_device->CreateBottomLevelAS(descs, flags);
}

std::shared_ptr<Resource> RenderDeviceImpl::CreateTopLevelAS(uint32_t instance_count, BuildAccelerationStructureFlags flags)
{
    return m_device->CreateTopLevelAS(instance_count, flags);
}

std::shared_ptr<View> RenderDeviceImpl::CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc)
{
    return m_device->CreateView(resource, view_desc);
}

std::shared_ptr<Shader> RenderDeviceImpl::CompileShader(const ShaderDesc& desc)
{
    return m_device->CompileShader(desc);
}

std::shared_ptr<Program> RenderDeviceImpl::CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders)
{
    return m_device->CreateProgram(shaders);
}

bool RenderDeviceImpl::IsDxrSupported() const
{
    return m_device->IsDxrSupported();
}

bool RenderDeviceImpl::IsVariableRateShadingSupported() const
{
    return m_device->IsVariableRateShadingSupported();
}

uint32_t RenderDeviceImpl::GetShadingRateImageTileSize() const
{
    return m_device->GetShadingRateImageTileSize();
}

void RenderDeviceImpl::ExecuteCommandLists(const std::vector<std::shared_ptr<RenderCommandList>>& command_lists)
{
    for (auto& command_list : command_lists)
    {
        command_list->GetFenceValue() = m_fence_value + 1;
    }
    ExecuteCommandListsImpl(command_lists);
    m_command_queue->Signal(m_fence, ++m_fence_value);
}

const std::string& RenderDeviceImpl::GetGpuName() const
{
    return m_adapter->GetName();
}

void RenderDeviceImpl::ExecuteCommandListsImpl(const std::vector<std::shared_ptr<RenderCommandList>>& command_lists)
{
    std::vector<std::shared_ptr<CommandList>> raw_command_lists;
    size_t patch_cmds = 0;
    for (size_t c = 0; c < command_lists.size(); ++c)
    {
        std::vector<ResourceBarrierDesc> new_barriers;
        decltype(auto) command_list_impl = command_lists[c]->As<RenderCommandListImpl>();
        auto barriers = command_list_impl.GetLazyBarriers();
        for (auto& barrier : barriers)
        {
            if (c == 0 && barrier.resource->AllowCommonStatePromotion(barrier.state_after))
                continue;
            decltype(auto) resource_base = barrier.resource->As<ResourceBase>();
            auto& global_state_tracker = resource_base.GetGlobalResourceStateTracker();
            if (global_state_tracker.HasResourceState() && barrier.base_mip_level == 0 && barrier.level_count == barrier.resource->GetLevelCount() &&
                barrier.base_array_layer == 0 && barrier.layer_count == barrier.resource->GetLayerCount())
            {
                barrier.state_before = global_state_tracker.GetResourceState();
                if (barrier.state_before != barrier.state_after)
                    new_barriers.emplace_back(barrier);
            }
            else
            {
                for (uint32_t i = 0; i < barrier.level_count; ++i)
                {
                    for (uint32_t j = 0; j < barrier.layer_count; ++j)
                    {
                        barrier.state_before = global_state_tracker.GetSubresourceState(barrier.base_mip_level + i, barrier.base_array_layer + j);
                        if (barrier.state_before != barrier.state_after)
                        {
                            auto& new_barrier = new_barriers.emplace_back(barrier);
                            new_barrier.base_mip_level = barrier.base_mip_level + i;
                            new_barrier.level_count = 1;
                            new_barrier.base_array_layer = barrier.base_array_layer + j;
                            new_barrier.layer_count = 1;
                        }
                    }
                }
            }
        }

        if (!new_barriers.empty())
        {
            std::shared_ptr<CommandList> tmp_cmd;
            if (c != 0 && kUseFakeClose)
            {
                tmp_cmd = command_lists[c - 1]->As<RenderCommandListImpl>().GetCommandList();
            }
            else
            {
                if (!m_fence_value_by_cmd.empty())
                {
                    auto& desc = m_fence_value_by_cmd.front();
                    if (m_fence->GetCompletedValue() >= desc.first)
                    {
                        m_fence->Wait(desc.first);
                        tmp_cmd = m_command_list_pool[desc.second];
                        tmp_cmd->Reset();
                        m_fence_value_by_cmd.pop_front();
                        m_fence_value_by_cmd.emplace_back(m_fence_value + 1, desc.second);
                    }
                }
                if (!tmp_cmd)
                {
                    tmp_cmd = m_command_list_pool.emplace_back(m_device->CreateCommandList(CommandListType::kGraphics));
                    m_fence_value_by_cmd.emplace_back(m_fence_value + 1, m_command_list_pool.size() - 1);
                }
                raw_command_lists.emplace_back(tmp_cmd);
            }

            tmp_cmd->ResourceBarrier(new_barriers);
            if (!kUseFakeClose)
                tmp_cmd->Close();
            ++patch_cmds;
        }

        auto& state_trackers = command_list_impl.GetResourceStateTrackers();
        for (const auto& state_tracker_pair : state_trackers)
        {
            auto& resource = state_tracker_pair.first;
            auto& state_tracker = state_tracker_pair.second;
            decltype(auto) resource_base = resource->As<ResourceBase>();
            auto& global_state_tracker = resource_base.GetGlobalResourceStateTracker();
            global_state_tracker.Merge(state_tracker);
        }

        raw_command_lists.emplace_back(command_list_impl.GetCommandList());
    }
    if (kUseFakeClose)
    {
        for (auto& cmd : raw_command_lists)
            cmd->Close();
    }
    m_command_queue->ExecuteCommandLists(raw_command_lists);
    if (patch_cmds)
        m_command_queue->Signal(m_fence, ++m_fence_value);
}

void RenderDeviceImpl::WaitForIdle()
{
    m_command_queue->Signal(m_fence, ++m_fence_value);
    m_fence->Wait(m_fence_value);
}

void RenderDeviceImpl::Resize(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    m_swapchain.reset();
    m_swapchain = m_device->CreateSwapchain(m_window, m_width, m_height, m_frame_count, m_vsync);
    m_frame_index = 0;
}

void RenderDeviceImpl::Present()
{
    auto& back_buffer = GetBackBuffer(GetFrameIndex());
    decltype(auto) resource_base = back_buffer->As<ResourceBase>();
    auto& global_state_tracker = resource_base.GetGlobalResourceStateTracker();
    ResourceBarrierDesc barrier = {};
    barrier.resource = back_buffer;
    barrier.state_before = global_state_tracker.GetSubresourceState(0, 0);
    barrier.state_after = ResourceState::kPresent;
    global_state_tracker.SetSubresourceState(0, 0, barrier.state_after);

    m_swapchain_command_list = m_swapchain_command_lists[m_frame_index];
    m_fence->Wait(m_swapchain_fence_values[m_frame_index]);
    m_swapchain_command_list->Reset();
    m_swapchain_command_list->ResourceBarrier({ barrier });
    m_swapchain_command_list->Close();

    m_swapchain->NextImage(m_fence, ++m_fence_value);
    m_command_queue->Wait(m_fence, m_fence_value);
    m_command_queue->ExecuteCommandLists({ m_swapchain_command_list });
    m_command_queue->Signal(m_fence, ++m_fence_value);
    m_swapchain_fence_values[m_frame_index] = m_fence_value;
    m_swapchain->Present(m_fence, m_fence_value);

    m_frame_index = (m_frame_index + 1) % m_frame_count;
}

void RenderDeviceImpl::Wait(uint64_t fence_value)
{
    m_fence->Wait(fence_value);
}

uint32_t RenderDeviceImpl::GetFrameIndex() const
{
    return m_frame_index;
}

std::shared_ptr<RenderDevice> CreateRenderDevice(const Settings& settings, GLFWwindow* window)
{
    return std::make_shared<RenderDeviceImpl>(settings, window);
}

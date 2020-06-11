#include "Context/Context.h"
#include <Utilities/FormatHelper.h>

Context::Context(const Settings& settings, GLFWwindow* window)
    : m_window(window)
{
    m_instance = CreateInstance(settings.api_type);
    m_adapter = std::move(m_instance->EnumerateAdapters()[settings.required_gpu_index]);
    m_device = m_adapter->CreateDevice();

    glfwGetWindowSize(window, &m_width, &m_height);
    m_swapchain = m_device->CreateSwapchain(window, m_width, m_height, FrameCount, settings.vsync);
    m_image_available_semaphore = m_device->CreateGPUSemaphore();
    m_rendering_finished_semaphore = m_device->CreateGPUSemaphore();

    for (uint32_t i = 0; i < FrameCount; ++i)
    {
        m_command_lists.emplace_back(std::make_shared<CommandListBox>(*m_device));
    }
    m_fence = m_device->CreateFence();
    m_command_list = m_command_lists.front();
    m_command_list->Open();
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

bool Context::IsDxrSupported() const
{
    return m_device->IsDxrSupported();
}

std::shared_ptr<Shader> Context::CompileShader(const ShaderDesc& desc)
{
    return m_device->CompileShader(desc);
}

std::shared_ptr<Program> Context::CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders)
{
    return m_device->CreateProgram(shaders);
}

std::shared_ptr<Resource> Context::GetBackBuffer()
{
    return m_swapchain->GetBackBuffer(m_frame_index);
}

void Context::Present()
{
    m_command_list->EndRenderPass();
    m_command_list->ResourceBarrier(GetBackBuffer(), ResourceState::kPresent);
    m_command_list->Close();
    m_fence->WaitAndReset();
    m_swapchain->NextImage(m_image_available_semaphore);
    m_device->Wait(m_image_available_semaphore);
    m_device->ExecuteCommandLists({ m_command_list->GetCommandList() }, m_fence);
    m_device->Signal(m_rendering_finished_semaphore);
    m_swapchain->Present(m_rendering_finished_semaphore);

    ++m_frame_index;
    m_frame_index %= FrameCount;
    m_command_list = m_command_lists[m_frame_index];
    m_command_list->Open();
}

std::shared_ptr<Device> Context::GetDevice()
{
    return m_device;
}

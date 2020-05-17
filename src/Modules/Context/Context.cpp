#include "Context/Context.h"

Context::Context(ApiType type, GLFWwindow* window)
    : m_window(window)
    , m_width(0)
    , m_height(0)
{
    glfwGetWindowSize(window, &m_width, &m_height);
}

std::unique_ptr<ProgramApi> Context::CreateProgram()
{
    return {};
}

std::shared_ptr<Resource> Context::CreateTexture(uint32_t bind_flag, gli::format format, uint32_t msaa_count, int width, int height, int depth, int mip_levels)
{
    return {};
}

std::shared_ptr<Resource> Context::CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride)
{
    return {};
}

std::shared_ptr<Resource> Context::CreateSampler(const SamplerDesc& desc)
{
    return {};
}

void Context::UpdateSubresource(const std::shared_ptr<Resource>& ires, uint32_t DstSubresource, const void* pSrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch)
{
}

void Context::SetViewport(float width, float height)
{
}

void Context::SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
}

void Context::IASetIndexBuffer(std::shared_ptr<Resource> res, gli::format Format)
{
}

void Context::IASetVertexBuffer(uint32_t slot, std::shared_ptr<Resource> res)
{
}

void Context::UseProgram(ProgramApi& program)
{
}

void Context::BeginEvent(const std::string& name)
{
}

void Context::EndEvent()
{
}

void Context::DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation)
{
}

void Context::Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
{
}

void Context::DispatchRays(uint32_t width, uint32_t height, uint32_t depth)
{
}

std::shared_ptr<Resource> Context::GetBackBuffer()
{
    return {};
}

void Context::Present()
{
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

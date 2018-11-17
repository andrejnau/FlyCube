#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "GLContext.h"
#include <glm/glm.hpp>
#include <gli/gli.hpp>
#include <glLoadGen/gl.h>
#include <iostream>
#include <Program/GLProgramApi.h>
#include "GLResource.h"

static void APIENTRY gl_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *msg, const void *data)
{
    std::cout << "debug call: " << msg << std::endl;
}

GLContext::GLContext(GLFWwindow* window, int width, int height)
    : Context(window, width, height)
{
    ogl_LoadFunctions();

    if (glDebugMessageCallback)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(gl_callback, nullptr);
    }

    auto version = glGetString(GL_VERSION);
    GLint num_extensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    for (GLint i = 0; i < num_extensions; ++i)
    {
        auto str = glGetStringi(GL_EXTENSIONS, i);
        std::cout << str << std::endl;
    }

    
    m_final_texture = CreateTexture(0, gli::format::FORMAT_RGBA8_UNORM_PACK8, 1, m_width, m_height);
    glCreateFramebuffers(1, &m_final_framebuffer);
    GLResource& res = static_cast<GLResource&>(*m_final_texture);
    glNamedFramebufferTexture(m_final_framebuffer, GL_COLOR_ATTACHMENT0, res.texture, 0);

    glEnable(GL_DEPTH_TEST);
    glDepthRange(0, 1);
}

std::unique_ptr<ProgramApi> GLContext::CreateProgram()
{
    return std::make_unique<GLProgramApi>(*this);
}

Resource::Ptr GLContext::CreateTexture(uint32_t bind_flag, gli::format format, uint32_t msaa_count, int width, int height, int depth, int mip_levels)
{
    GLResource::Ptr res = std::make_shared<GLResource>();

    gli::gl GL(gli::gl::PROFILE_GL32);
    gli::swizzles swizzle(gli::SWIZZLE_RED, gli::SWIZZLE_GREEN, gli::SWIZZLE_BLUE, gli::SWIZZLE_ALPHA);
    gli::gl::format gl_format = GL.translate(format, swizzle);

    if (depth == 6)
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &res->texture);
    else
        glCreateTextures(GL_TEXTURE_2D, 1, &res->texture);

    glTextureStorage2D(res->texture, mip_levels, gl_format.Internal, width, height);

    res->image.int_format = gl_format.Internal;
    res->image.format = gl_format.External;
    res->image.type = gl_format.Type;
    res->image.level_count = mip_levels;
    res->image.width = width;
    res->image.height = height;
    res->image.is_compressed = gli::is_compressed(format);

    return res;
}

Resource::Ptr GLContext::CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride)
{
    GLResource::Ptr res = std::make_shared<GLResource>();

    glCreateBuffers(1, &res->buffer);
    if (buffer_size)
        glNamedBufferStorage(res->buffer, buffer_size, nullptr, GL_DYNAMIC_STORAGE_BIT);
    res->res_type = GLResource::Type::kBuffer;
    res->buffer_size = buffer_size;

    return res;
}

Resource::Ptr GLContext::CreateSampler(const SamplerDesc & desc)
{
    return {};
}

void GLContext::UpdateSubresource(const Resource::Ptr & ires, uint32_t DstSubresource, const void * pSrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch)
{
    GLResource& res = static_cast<GLResource&>(*ires);

    if (res.res_type == GLResource::Type::kBuffer)
    {
        glNamedBufferSubData(res.buffer, 0, res.buffer_size, pSrcData);
    }
    else
    {
        if (res.image.is_compressed)
        {
            glCompressedTextureSubImage2D(res.texture, DstSubresource, 0, 0, res.image.width >> DstSubresource, res.image.height >> DstSubresource, res.image.int_format, SrcDepthPitch, pSrcData);
        }
        else
        {
            glTextureSubImage2D(res.texture, DstSubresource, 0, 0, res.image.width >> DstSubresource, res.image.height >> DstSubresource, res.image.format, res.image.type, pSrcData);
        }
    }
}

void GLContext::SetViewport(float width, float height)
{
    glViewport(0, 0, width, height);
}

void GLContext::SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
}

void GLContext::IASetIndexBuffer(Resource::Ptr ires, uint32_t SizeInBytes, gli::format Format)
{
    gli::gl GL(gli::gl::PROFILE_GL32);
    gli::swizzles swizzle(gli::SWIZZLE_RED, gli::SWIZZLE_GREEN, gli::SWIZZLE_BLUE, gli::SWIZZLE_ALPHA);
    gli::gl::format gl_format = GL.translate(Format, swizzle);
    m_ibo_type = gl_format.Type;
    m_ibo_size = gli::block_size(Format);
    GLResource& res = static_cast<GLResource&>(*ires);
    glVertexArrayElementBuffer(m_current_program->m_vao, res.buffer);
}

void GLContext::IASetVertexBuffer(uint32_t slot, Resource::Ptr ires, uint32_t SizeInBytes, uint32_t Stride)
{
    GLResource& res = static_cast<GLResource&>(*ires);
    glVertexArrayVertexBuffer(m_current_program->m_vao, slot, res.buffer, 0, Stride);
}

void GLContext::BeginEvent(LPCWSTR Name)
{
    std::string name = wstring_to_utf8(Name);
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name.c_str());
}

void GLContext::EndEvent()
{
    glPopDebugGroup();
}

void GLContext::DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation)
{
    m_current_program->ApplyBindings();
    glDrawElementsBaseVertex(GL_TRIANGLES, IndexCount, m_ibo_type, (void*)(m_ibo_size * StartIndexLocation), BaseVertexLocation);
}

void GLContext::Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
{
    m_current_program->ApplyBindings();
    glDispatchCompute(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

Resource::Ptr GLContext::GetBackBuffer()
{
    return m_final_texture;
}

void GLContext::Present(const Resource::Ptr& ires)
{
    glBlitNamedFramebuffer(m_final_framebuffer, 0, 0, 0, m_width, m_height, 0, m_height, m_width, 0, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void GLContext::ResizeBackBuffer(int width, int height)
{
}

void GLContext::UseProgram(GLProgramApi & program_api)
{
    m_current_program = std::addressof(program_api);
}

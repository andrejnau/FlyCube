#pragma once

#include "Context/Context.h"
#include "Context/VKDescriptorPool.h"
#include <GLFW/glfw3.h>
#include <Geometry/IABuffer.h>
#include <assimp/postprocess.h>
#include <glLoadGen/gl.h>

class GLProgramApi;
class GLContext : public Context
{
public:
    GLContext(GLFWwindow* window);

    virtual std::unique_ptr<ProgramApi> CreateProgram() override;
    virtual Resource::Ptr CreateTexture(uint32_t bind_flag, gli::format format, uint32_t msaa_count, int width, int height, int depth = 1, int mip_levels = 1) override;
    virtual Resource::Ptr CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride) override;
    virtual Resource::Ptr CreateSampler(const SamplerDesc& desc) override;
    virtual void UpdateSubresource(const Resource::Ptr& ires, uint32_t DstSubresource, const void *pSrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch) override;

    virtual void SetViewport(float width, float height) override;
    virtual void SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom) override;

    virtual void IASetIndexBuffer(Resource::Ptr res, gli::format Format) override;
    virtual void IASetVertexBuffer(uint32_t slot, Resource::Ptr res) override;

    virtual void BeginEvent(const std::string& name) override;
    virtual void EndEvent() override;

    virtual void DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation) override;
    virtual void Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ) override;

    virtual Resource::Ptr GetBackBuffer() override;
    virtual void Present() override;

    virtual void ResizeBackBuffer(int width, int height) override;

    void UseProgram(GLProgramApi& program_api);
    GLProgramApi* m_current_program = nullptr;
    Resource::Ptr m_final_texture;
    GLuint m_final_framebuffer = 0;
    GLenum m_ibo_type = GL_NONE;
    uint32_t m_ibo_size = 0;
};

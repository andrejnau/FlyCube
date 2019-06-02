#pragma once

#include <wrl.h>
#include <dxgiformat.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <array>

#include <Program/ProgramApi.h>
#include "Context/BaseTypes.h"
#include <Resource/Resource.h>
#include <glm/glm.hpp>
#include <gli/gli.hpp>

using namespace Microsoft::WRL;

class Context
{
public:
    Context(GLFWwindow* window);
    virtual ~Context() {}

    virtual std::unique_ptr<ProgramApi> CreateProgram() = 0;
    virtual Resource::Ptr CreateTexture(uint32_t bind_flag, gli::format format, uint32_t msaa_count, int width, int height, int depth = 1, int mip_levels = 1) = 0;
    virtual Resource::Ptr CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride) = 0;
    virtual Resource::Ptr CreateSampler(const SamplerDesc& desc) = 0;
    virtual Resource::Ptr CreateBottomLevelAS(const BufferDesc& vertex);
    virtual Resource::Ptr CreateBottomLevelAS(const BufferDesc& vertex, const BufferDesc& index);
    virtual Resource::Ptr CreateTopLevelAS(const std::vector<std::pair<Resource::Ptr, glm::mat4>>& geometry);
    virtual void UpdateSubresource(const Resource::Ptr& ires, uint32_t DstSubresource, const void *pSrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch) = 0;

    virtual void SetViewport(float width, float height) = 0;
    virtual void SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom) = 0;

    virtual void IASetIndexBuffer(Resource::Ptr res, gli::format Format) = 0;
    virtual void IASetVertexBuffer(uint32_t slot, Resource::Ptr res) = 0;

    virtual void UseProgram(ProgramApi& program) = 0;

    virtual void BeginEvent(const std::string& name) = 0;
    virtual void EndEvent() = 0;

    virtual void DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation) = 0;
    virtual void Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ) = 0;
    virtual void DispatchRays(uint32_t width, uint32_t height, uint32_t depth) {}

    virtual Resource::Ptr GetBackBuffer() = 0;
    virtual void Present() = 0;

    virtual bool IsDxrSupported() const { return false; }

    virtual void OnDestroy() {}

    void OnResize(int width, int height);
    size_t GetFrameIndex() const;
    GLFWwindow* GetWindow();

    static constexpr size_t FrameCount = 3;
  
protected:
    virtual void ResizeBackBuffer(int width, int height) = 0;
    int m_width;
    int m_height;
    GLFWwindow* m_window;
    uint32_t m_frame_index = 0;
};

template <typename T>
class PerFrameData
{
public:
    PerFrameData(Context& context)
        : m_context(context)
    {
    }

    T& get()
    {
        return m_data[m_context.GetFrameIndex()];
    }

    T& operator[](size_t pos)
    {
        return m_data[pos];
    }
private:
    Context& m_context;
    std::array<T, Context::FrameCount> m_data;
};

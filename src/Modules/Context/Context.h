#pragma once
#include <Instance/BaseTypes.h>
#include <memory>
#include <Scene/IScene.h>
#include <ApiType/ApiType.h>

#include <GLFW/glfw3.h>
#include <memory>
#include <array>

#include <ProgramApi/ProgramApi.h>
#include <Resource/Resource.h>
#include <glm/glm.hpp>
#include <gli/gli.hpp>

class Context
{
public:
    Context(ApiType type, GLFWwindow* window);
    ~Context() {}

    std::unique_ptr<ProgramApi> CreateProgram();
    std::shared_ptr<Resource> CreateTexture(uint32_t bind_flag, gli::format format, uint32_t msaa_count, int width, int height, int depth = 1, int mip_levels = 1);
    std::shared_ptr<Resource> CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride);
    std::shared_ptr<Resource> CreateSampler(const SamplerDesc& desc);
    void UpdateSubresource(const std::shared_ptr<Resource>& ires, uint32_t DstSubresource, const void *pSrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch);

    void SetViewport(float width, float height);
    void SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom);

    void IASetIndexBuffer(std::shared_ptr<Resource> res, gli::format Format);
    void IASetVertexBuffer(uint32_t slot, std::shared_ptr<Resource> res);

    void UseProgram(ProgramApi& program);

    void BeginEvent(const std::string& name);
    void EndEvent();

    void DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation);
    void Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ);
    void DispatchRays(uint32_t width, uint32_t height, uint32_t depth);

    std::shared_ptr<Resource> GetBackBuffer();
    void Present();

    bool IsDxrSupported() const { return false; }

    void OnDestroy() {}

    void OnResize(int width, int height);
    size_t GetFrameIndex() const;
    GLFWwindow* GetWindow();

    static constexpr size_t FrameCount = 3;

protected:
    void ResizeBackBuffer(int width, int height);
    int m_width;
    int m_height;
    GLFWwindow* m_window;
    uint32_t m_frame_index;
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

#pragma once
#include <Instance/BaseTypes.h>
#include <Instance/Instance.h>
#include <AppBox/Settings.h>
#include <memory>
#include <Scene/IScene.h>
#include <ApiType/ApiType.h>

#include <GLFW/glfw3.h>
#include <memory>
#include <array>
#include <set>

#include <Resource/Resource.h>
#include <glm/glm.hpp>
#include <gli/gli.hpp>

class ProgramApi;

class Context
{
public:
    Context(const Settings& settings, GLFWwindow* window);
    ~Context() {}

    std::shared_ptr<ProgramApi> CreateProgramApi();

    void ApplyBindings();
    void Attach(const NamedBindKey& bind_key, const std::shared_ptr<Resource>& resource, const LazyViewDesc& view_desc);
    void ClearRenderTarget(uint32_t slot, const std::array<float, 4>& color);
    void ClearDepthStencil(uint32_t ClearFlags, float Depth, uint8_t Stencil);
    void SetRasterizeState(const RasterizerDesc& desc);
    void SetBlendState(const BlendDesc& desc);
    void SetDepthStencilState(const DepthStencilDesc& desc);

    void UseProgram(ProgramApi& program_api);

    std::shared_ptr<Program> CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders)
    {
        return m_device->CreateProgram(shaders);
    }

    std::shared_ptr<Shader> CompileShader(const ShaderDesc& desc);

    std::shared_ptr<Resource> CreateTexture(uint32_t bind_flag, gli::format format, uint32_t msaa_count, int width, int height, int depth = 1, int mip_levels = 1);
    std::shared_ptr<Resource> CreateBuffer(uint32_t bind_flag, uint32_t buffer_size);
    std::shared_ptr<Resource> CreateSampler(const SamplerDesc& desc);
    std::shared_ptr<Resource> CreateBottomLevelAS(const BufferDesc& vertex, const BufferDesc& index = {});
    std::shared_ptr<Resource> CreateTopLevelAS(const std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4>>& geometry);
    void UpdateSubresource(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch = 0, uint32_t depth_pitch = 0);

    void SetViewport(float width, float height);
    void SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom);

    void IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format);
    void IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource);

    void BeginEvent(const std::string& name);
    void EndEvent();

    void DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation);
    void Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ);
    void DispatchRays(uint32_t width, uint32_t height, uint32_t depth);

    std::shared_ptr<Resource> GetBackBuffer();
    void Present();

    void ResourceBarrier(const std::shared_ptr<Resource>& resource, ResourceState state);

    bool IsDxrSupported() const;

    void OnResize(int width, int height);
    size_t GetFrameIndex() const;
    GLFWwindow* GetWindow();

    static constexpr size_t FrameCount = 3;

    void ResizeBackBuffer(int width, int height);
    GLFWwindow* m_window;
    uint32_t m_frame_index = 0;
    int m_window_width = 0;
    int m_window_height = 0;
    std::shared_ptr<Instance> m_instance;
    std::shared_ptr<Adapter> m_adapter;
    std::shared_ptr<Device> m_device;
    std::shared_ptr<Swapchain> m_swapchain;
    std::vector<std::shared_ptr<CommandList>> m_command_lists;
    std::shared_ptr<CommandList> m_command_list;
    std::shared_ptr<Fence> m_fence;
    std::shared_ptr<Semaphore> m_image_available_semaphore;
    std::shared_ptr<Semaphore> m_rendering_finished_semaphore;
    bool m_is_open_render_pass = false;
    uint32_t m_viewport_width = 0;
    uint32_t m_viewport_height = 0;

private:
    void Attach(const BindKey& bind_key, const std::shared_ptr<View>& view);
    void OnAttachSRV(const BindKey& bind_key, const std::shared_ptr<View>& view);
    void OnAttachUAV(const BindKey& bind_key, const std::shared_ptr<View>& view);
    void OnAttachRTV(const BindKey& bind_key, const std::shared_ptr<View>& view);
    void OnAttachDSV(const BindKey& bind_key, const std::shared_ptr<View>& view);

    std::shared_ptr<View> CreateView(const BindKey& bind_key, const std::shared_ptr<Resource>& resource, const LazyViewDesc& view_desc);
    std::shared_ptr<View> FindView(ShaderType shader_type, ViewType view_type, uint32_t slot);

    void SetBinding(const BindKey& bind_key, const std::shared_ptr<View>& view);

    struct BoundResource
    {
        std::shared_ptr<Resource> res;
        std::shared_ptr<View> view;
    };
    std::map<BindKey, BoundResource> m_bound_resources;

    std::vector<std::shared_ptr<Shader>> m_shaders;
    std::shared_ptr<Program> m_program;
    ProgramApi* m_program_api;
    std::shared_ptr<Pipeline> m_pipeline;
    std::shared_ptr<BindingSet> m_binding_set;

    std::map<GraphicsPipelineDesc, std::shared_ptr<Pipeline>> m_pso;
    std::map<ComputePipelineDesc, std::shared_ptr<Pipeline>> m_compute_pso;
    std::map<std::tuple<uint32_t, uint32_t, std::vector<std::shared_ptr<View>>, std::shared_ptr<View>>, std::shared_ptr<Framebuffer>> m_framebuffers;
    std::shared_ptr<Framebuffer> m_framebuffer;
    std::map<std::tuple<BindKey, std::shared_ptr<Resource>, LazyViewDesc>, std::shared_ptr<View>> m_views;
    ComputePipelineDesc m_compute_pipeline_desc;
    GraphicsPipelineDesc m_graphic_pipeline_desc;
    std::vector<std::shared_ptr<ProgramApi>> m_created_program;
    std::map<BindKey, std::string> m_binding_names;

    void UpdateSubresourceDefault(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch, uint32_t depth_pitch);
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

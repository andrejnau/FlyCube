#pragma once
#include "RenderDevice/RenderDevice.h"
#include <Instance/BaseTypes.h>
#include <Instance/Instance.h>
#include <AppBox/Settings.h>
#include <memory>
#include <ApiType/ApiType.h>

#include <GLFW/glfw3.h>
#include <memory>
#include <array>
#include <set>

#include <Resource/Resource.h>
#include <glm/glm.hpp>
#include <gli/gli.hpp>

#include <RenderCommandList/RenderCommandList.h>
#include <RenderDevice/RenderDevice.h>
#include <ObjectCache/ObjectCache.h>

class RenderDeviceImpl : public RenderDevice
{
public:
    RenderDeviceImpl(const Settings& settings, GLFWwindow* window);
    ~RenderDeviceImpl();

    std::shared_ptr<RenderCommandList> CreateRenderCommandList(CommandListType type) override;
    std::shared_ptr<Resource> CreateTexture(uint32_t bind_flag, gli::format format, uint32_t sample_count, int width, int height, int depth, int mip_levels) override;
    std::shared_ptr<Resource> CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, MemoryType memory_type) override;
    std::shared_ptr<Resource> CreateSampler(const SamplerDesc& desc) override;
    std::shared_ptr<Resource> CreateBottomLevelAS(const std::vector<RaytracingGeometryDesc>& descs, BuildAccelerationStructureFlags flags) override;
    std::shared_ptr<Resource> CreateTopLevelAS(uint32_t instance_count, BuildAccelerationStructureFlags flags) override;
    std::shared_ptr<View> CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc) override;
    std::shared_ptr<Shader> CompileShader(const ShaderDesc& desc) override;
    std::shared_ptr<Program> CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders) override;
    bool IsDxrSupported() const override;
    bool IsRayQuerySupported() const override;
    bool IsVariableRateShadingSupported() const override;
    bool IsMeshShadingSupported() const override;
    uint32_t GetShadingRateImageTileSize() const override;
    uint32_t GetFrameIndex() const override;
    gli::format GetFormat() const override;
    std::shared_ptr<Resource> GetBackBuffer(uint32_t buffer) override;
    const std::string& GetGpuName() const override;
    void ExecuteCommandLists(const std::vector<std::shared_ptr<RenderCommandList>>& command_lists) override;
    void Present() override;
    void Wait(uint64_t fence_value) override;
    void WaitForIdle() override;
    void Resize(uint32_t width, uint32_t height) override;

    void ExecuteCommandListsImpl(const std::vector<std::shared_ptr<RenderCommandList>>& command_lists);

private:
    GLFWwindow* m_window;
    bool m_vsync;
    uint32_t m_frame_count;
    uint32_t m_frame_index = 0;
    int m_width = 0;
    int m_height = 0;
    std::shared_ptr<Instance> m_instance;
    std::shared_ptr<Adapter> m_adapter;
    std::shared_ptr<Device> m_device;
    std::shared_ptr<CommandQueue> m_command_queue;
    std::shared_ptr<Swapchain> m_swapchain;
    std::vector<std::shared_ptr<CommandList>> m_swapchain_command_lists;
    std::vector<uint64_t> m_swapchain_fence_values;
    std::shared_ptr<CommandList> m_swapchain_command_list;
    uint64_t m_fence_value = 0;
    std::shared_ptr<Fence> m_fence;
    std::vector<std::shared_ptr<CommandList>> m_command_list_pool;
    std::deque<std::pair<uint64_t /*fence_value*/, size_t /*offset*/>> m_fence_value_by_cmd;
    std::unique_ptr<ObjectCache> m_object_cache;
};

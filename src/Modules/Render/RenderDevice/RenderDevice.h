#pragma once
#include <RenderCommandList/RenderCommandList.h>
#include <Instance/QueryInterface.h>
#include <Instance/BaseTypes.h>
#include <Swapchain/Swapchain.h>
#include <CommandList/CommandList.h>
#include <Fence/Fence.h>
#include <Instance/BaseTypes.h>
#include <Program/Program.h>
#include <Framebuffer/Framebuffer.h>
#include <Pipeline/Pipeline.h>
#include <Shader/Shader.h>
#include <RenderPass/RenderPass.h>
#include <CommandQueue/CommandQueue.h>
#include <memory>
#include <vector>
#include <AppBox/Settings.h>
#include <gli/format.hpp>

class RenderDevice : public QueryInterface
{
public:
    virtual ~RenderDevice() = default;
    virtual std::shared_ptr<RenderCommandList> CreateRenderCommandList(CommandListType type = CommandListType::kGraphics) = 0;
    virtual std::shared_ptr<Resource> CreateTexture(uint32_t bind_flag, gli::format format, uint32_t sample_count, int width, int height, int depth = 1, int mip_levels = 1) = 0;
    virtual std::shared_ptr<Resource> CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, MemoryType memory_type = MemoryType::kDefault) = 0;
    virtual std::shared_ptr<Resource> CreateSampler(const SamplerDesc& desc) = 0;
    virtual std::shared_ptr<Resource> CreateBottomLevelAS(const std::vector<RaytracingGeometryDesc>& descs, BuildAccelerationStructureFlags flags) = 0;
    virtual std::shared_ptr<Resource> CreateTopLevelAS(uint32_t instance_count, BuildAccelerationStructureFlags flags) = 0;
    virtual std::shared_ptr<View> CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc) = 0;
    virtual std::shared_ptr<Shader> CompileShader(const ShaderDesc& desc) = 0;
    virtual std::shared_ptr<Program> CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders) = 0;
    virtual bool IsDxrSupported() const = 0;
    virtual bool IsRayQuerySupported() const = 0;
    virtual bool IsVariableRateShadingSupported() const = 0;
    virtual bool IsMeshShadingSupported() const = 0;
    virtual bool IsDrawIndirectCountSupported() const = 0;
    virtual bool IsGeometryShaderSupported() const = 0;
    virtual uint32_t GetShadingRateImageTileSize() const = 0;
    virtual uint32_t GetFrameIndex() const = 0;
    virtual gli::format GetFormat() const = 0;
    virtual std::shared_ptr<Resource> GetBackBuffer(uint32_t buffer) = 0;
    virtual const std::string& GetGpuName() const = 0;
    virtual void ExecuteCommandLists(const std::vector<std::shared_ptr<RenderCommandList>>& command_lists) = 0;
    virtual void Present() = 0;
    virtual void Wait(uint64_t fence_value) = 0;
    virtual void WaitForIdle() = 0;
    virtual void Resize(uint32_t width, uint32_t height) = 0;
};

std::shared_ptr<RenderDevice> CreateRenderDevice(const Settings& settings, Window window, uint32_t width, uint32_t height);

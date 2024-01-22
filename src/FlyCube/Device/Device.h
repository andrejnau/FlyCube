#pragma once
#include "BindingSetLayout/BindingSetLayout.h"
#include "CommandList/CommandList.h"
#include "CommandQueue/CommandQueue.h"
#include "Fence/Fence.h"
#include "Framebuffer/Framebuffer.h"
#include "Instance/BaseTypes.h"
#include "Instance/QueryInterface.h"
#include "Memory/Memory.h"
#include "Pipeline/Pipeline.h"
#include "Program/Program.h"
#include "QueryHeap/QueryHeap.h"
#include "RenderPass/RenderPass.h"
#include "Shader/Shader.h"
#include "Swapchain/Swapchain.h"

#include <gli/format.hpp>

#include <memory>
#include <vector>

struct MemoryBudget {
    uint64_t budget;
    uint64_t usage;
};

class Device : public QueryInterface {
public:
    virtual ~Device() = default;
    virtual std::shared_ptr<Memory> AllocateMemory(uint64_t size,
                                                   MemoryType memory_type,
                                                   uint32_t memory_type_bits) = 0;
    virtual std::shared_ptr<CommandQueue> GetCommandQueue(CommandListType type) = 0;
    virtual uint32_t GetTextureDataPitchAlignment() const = 0;
    virtual std::shared_ptr<Swapchain> CreateSwapchain(WindowHandle window,
                                                       uint32_t width,
                                                       uint32_t height,
                                                       uint32_t frame_count,
                                                       bool vsync) = 0;
    virtual std::shared_ptr<CommandList> CreateCommandList(CommandListType type) = 0;
    virtual std::shared_ptr<Fence> CreateFence(uint64_t initial_value) = 0;
    virtual std::shared_ptr<Resource> CreateTexture(TextureType type,
                                                    uint32_t bind_flag,
                                                    gli::format format,
                                                    uint32_t sample_count,
                                                    int width,
                                                    int height,
                                                    int depth,
                                                    int mip_levels) = 0;
    virtual std::shared_ptr<Resource> CreateBuffer(uint32_t bind_flag, uint32_t buffer_size) = 0;
    virtual std::shared_ptr<Resource> CreateSampler(const SamplerDesc& desc) = 0;
    virtual std::shared_ptr<View> CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc) = 0;
    virtual std::shared_ptr<BindingSetLayout> CreateBindingSetLayout(const std::vector<BindKey>& descs) = 0;
    virtual std::shared_ptr<BindingSet> CreateBindingSet(const std::shared_ptr<BindingSetLayout>& layout) = 0;
    virtual std::shared_ptr<RenderPass> CreateRenderPass(const RenderPassDesc& desc) = 0;
    virtual std::shared_ptr<Framebuffer> CreateFramebuffer(const FramebufferDesc& desc) = 0;
    virtual std::shared_ptr<Shader> CreateShader(const std::vector<uint8_t>& blob,
                                                 ShaderBlobType blob_type,
                                                 ShaderType shader_type) = 0;
    virtual std::shared_ptr<Shader> CompileShader(const ShaderDesc& desc) = 0;
    virtual std::shared_ptr<Program> CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders) = 0;
    virtual std::shared_ptr<Pipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
    virtual std::shared_ptr<Pipeline> CreateComputePipeline(const ComputePipelineDesc& desc) = 0;
    virtual std::shared_ptr<Pipeline> CreateRayTracingPipeline(const RayTracingPipelineDesc& desc) = 0;
    virtual std::shared_ptr<Resource> CreateAccelerationStructure(AccelerationStructureType type,
                                                                  const std::shared_ptr<Resource>& resource,
                                                                  uint64_t offset) = 0;
    virtual std::shared_ptr<QueryHeap> CreateQueryHeap(QueryHeapType type, uint32_t count) = 0;
    virtual bool IsDxrSupported() const = 0;
    virtual bool IsRayQuerySupported() const = 0;
    virtual bool IsVariableRateShadingSupported() const = 0;
    virtual bool IsMeshShadingSupported() const = 0;
    virtual bool IsDrawIndirectCountSupported() const = 0;
    virtual bool IsGeometryShaderSupported() const = 0;
    virtual uint32_t GetShadingRateImageTileSize() const = 0;
    virtual MemoryBudget GetMemoryBudget() const = 0;
    virtual uint32_t GetShaderGroupHandleSize() const = 0;
    virtual uint32_t GetShaderRecordAlignment() const = 0;
    virtual uint32_t GetShaderTableAlignment() const = 0;
    virtual RaytracingASPrebuildInfo GetBLASPrebuildInfo(const std::vector<RaytracingGeometryDesc>& descs,
                                                         BuildAccelerationStructureFlags flags) const = 0;
    virtual RaytracingASPrebuildInfo GetTLASPrebuildInfo(uint32_t instance_count,
                                                         BuildAccelerationStructureFlags flags) const = 0;
    virtual ShaderBlobType GetSupportedShaderBlobType() const = 0;
};

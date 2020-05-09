#pragma once

#include <vulkan/vulkan.hpp>
#include "Context/Context.h"
#include "Context/VKDescriptorPool.h"
#include <GLFW/glfw3.h>
#include <Geometry/IABuffer.h>
#include <assimp/postprocess.h>

struct VKProgramApi;
class VKContext
    : public Context
    , public VKResource::Destroyer
{
public:
    void CreateInstance();
    void SelectQueueFamilyIndex();
    void CreateDevice();
    void CreateSwapchain(int width, int height);
    void SelectPhysicalDevice();
    VKContext(GLFWwindow* window);

    virtual std::unique_ptr<ProgramApi> CreateProgram() override;
    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    virtual Resource::Ptr CreateTexture(uint32_t bind_flag, gli::format format, uint32_t msaa_count, int width, int height, int depth = 1, int mip_levels = 1) override;
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    virtual Resource::Ptr CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride) override;
    virtual Resource::Ptr CreateSampler(const SamplerDesc& desc) override;
    void TransitionImageLayout(VKResource::Image& image, vk::ImageLayout newLayout, const ViewDesc& view_desc);
    vk::ImageAspectFlags GetAspectFlags(vk::Format format);
    virtual void UpdateSubresource(const Resource::Ptr& ires, uint32_t DstSubresource, const void *pSrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch) override;

    virtual void SetViewport(float width, float height) override;
    virtual void SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom) override;

    virtual Resource::Ptr CreateBottomLevelAS(const BufferDesc& vertex) override;
    virtual Resource::Ptr CreateBottomLevelAS(const BufferDesc& vertex, const BufferDesc& index) override;
    virtual Resource::Ptr CreateTopLevelAS(const std::vector<std::pair<Resource::Ptr, glm::mat4>>& geometry) override;
    virtual void UseProgram(ProgramApi& program) override;

    virtual void IASetIndexBuffer(Resource::Ptr res, gli::format Format) override;
    virtual void IASetVertexBuffer(uint32_t slot, Resource::Ptr res) override;

    virtual void BeginEvent(const std::string& name) override;
    virtual void EndEvent() override;

    virtual void DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation) override;
    virtual void Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ) override;
    virtual void DispatchRays(uint32_t width, uint32_t height, uint32_t depth) override;

    virtual Resource::Ptr GetBackBuffer() override;
    void CloseCommandBuffer();
    void Submit();
    void SwapBuffers();
    void OpenCommandBuffer();
    virtual void Present() override;

    virtual void ResizeBackBuffer(int width, int height) override;

    virtual bool IsDxrSupported() const { return true; }

    VKProgramApi* m_current_program = nullptr;

    vk::UniqueInstance m_instance;
    vk::PhysicalDevice m_physical_device;
    uint32_t m_queue_family_index = 0;
    vk::UniqueDevice m_device;
    vk::Queue m_queue;
    vk::UniqueSurfaceKHR m_surface;
    vk::Format m_swapchain_color_format = vk::Format::eB8G8R8Unorm;
    vk::UniqueSwapchainKHR m_swapchain;
    std::vector<vk::Image> m_images;
    vk::UniqueCommandPool m_cmd_pool;
    std::vector<vk::UniqueCommandBuffer> m_cmd_bufs;
    vk::UniqueSemaphore m_image_available_semaphore;
    vk::UniqueSemaphore m_rendering_finished_semaphore;
    vk::UniqueFence m_fence;

    VKDescriptorPool& GetDescriptorPool();
    std::unique_ptr<VKDescriptorPool> descriptor_pool[FrameCount];

    vk::RenderPass m_render_pass;
    vk::Framebuffer m_framebuffer;
    bool m_is_open_render_pass = false;

    std::vector<std::reference_wrapper<VKProgramApi>> m_created_program;
    VKResource::Ptr m_back_buffers[FrameCount];

    void QueryOnDelete(VKResource res) override
    {
        m_deletion_queue[m_frame_index].emplace_back(std::move(res));
    }

    std::vector<VKResource> m_deletion_queue[FrameCount];
};


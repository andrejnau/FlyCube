#pragma once

#include "Context/Context.h"
#include "Context/VKDescriptorPool.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <Geometry/IABuffer.h>
#include <assimp/postprocess.h>

struct VKProgramApi;
class VKContext : public Context
{
public:
    void CreateInstance();
    void SelectQueueFamilyIndex();
    void CreateDevice();
    void CreateSwapchain(int width, int height);
    void SelectPhysicalDevice();
    VKContext(GLFWwindow* window);

    virtual std::unique_ptr<ProgramApi> CreateProgram() override;
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    virtual Resource::Ptr CreateTexture(uint32_t bind_flag, gli::format format, uint32_t msaa_count, int width, int height, int depth = 1, int mip_levels = 1) override;
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    virtual Resource::Ptr CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride) override;
    virtual Resource::Ptr CreateSampler(const SamplerDesc& desc) override;
    void TransitionImageLayout(VKResource::Image& image, VkImageLayout newLayout, const ViewDesc& view_desc);
    VkImageAspectFlags GetAspectFlags(VkFormat format);
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
    void CloseCommandBuffer();
    void Submit();
    void SwapBuffers();
    void OpenCommandBuffer();
    virtual void Present() override;

    virtual void ResizeBackBuffer(int width, int height) override;

    void UseProgram(VKProgramApi& program_api);
    VKProgramApi* m_current_program = nullptr;


    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
    uint32_t m_queue_family_index = 0;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_queue = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkFormat m_swapchain_color_format = VK_FORMAT_B8G8R8_UNORM;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> m_images;
    VkCommandPool m_cmd_pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_cmd_bufs;
    VkSemaphore m_image_available_semaphore = VK_NULL_HANDLE;
    VkSemaphore m_rendering_finished_semaphore = VK_NULL_HANDLE;
    VkFence m_fence = VK_NULL_HANDLE;;

    VKDescriptorPool& GetDescriptorPool();
    std::unique_ptr<VKDescriptorPool> descriptor_pool[FrameCount];

    VkRenderPass m_render_pass = VK_NULL_HANDLE;
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
    bool m_is_open_render_pass = false;
    VKResource::Ptr m_final_rt;

    std::vector<std::reference_wrapper<VKProgramApi>> m_created_program;
};

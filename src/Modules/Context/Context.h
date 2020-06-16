#pragma once
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

#include <CommandListBox/CommandListBox.h>

class Context
{
public:
    Context(const Settings& settings, GLFWwindow* window);
    ~Context();

    GLFWwindow* GetWindow()
    {
        return m_window;
    }

    std::shared_ptr<CommandListBox> CreateCommandList(bool open = false)
    {
        auto cmd = std::make_shared<CommandListBox>(*m_device);
        if (open)
            cmd->Open();
        return cmd;
    }

    void ExecuteCommandLists(const std::vector<std::shared_ptr<CommandListBox>>& command_lists);

    std::shared_ptr<Resource> GetBackBuffer(uint32_t buffer);
    void WaitIdle();
    void Resize(uint32_t width, uint32_t height);
    void Present();

    std::shared_ptr<Device> GetDevice();
    uint32_t GetFrameIndex()
    {
        return m_frame_index;
    }

    static constexpr int FrameCount = 3;

private:
    GLFWwindow* m_window;
    int m_width = 0;
    int m_height = 0;
    uint32_t m_frame_index = 0;
    bool m_vsync = false;

    std::shared_ptr<Instance> m_instance;
    std::shared_ptr<Adapter> m_adapter;
    std::shared_ptr<Device> m_device;
    std::shared_ptr<Swapchain> m_swapchain;
    std::shared_ptr<Semaphore> m_image_available_semaphore;
    std::shared_ptr<Semaphore> m_rendering_finished_semaphore;
    std::vector<std::shared_ptr<CommandList>> m_swapchain_command_lists;
    std::shared_ptr<CommandList> m_swapchain_command_list;
    std::shared_ptr<Fence> m_fence;
    std::array<std::vector<std::shared_ptr<CommandList>>, FrameCount> m_tmp_command_lists;
    std::array<uint32_t, FrameCount> m_tmp_command_lists_offset = {};
};

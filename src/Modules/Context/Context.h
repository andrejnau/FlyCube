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

#include <CommandListBox/CommandListBox.h>

class Context
{
public:
    Context(const Settings& settings, GLFWwindow* window);

    GLFWwindow* GetWindow()
    {
        return m_window;
    }

    CommandListBox* operator->();

    bool IsDxrSupported() const;

    std::shared_ptr<Resource> CreateTexture(uint32_t bind_flag, gli::format format, uint32_t msaa_count, int width, int height, int depth = 1, int mip_levels = 1);
    std::shared_ptr<Resource> CreateBuffer(uint32_t bind_flag, uint32_t buffer_size);
    std::shared_ptr<Resource> CreateSampler(const SamplerDesc& desc);

    std::shared_ptr<Shader> CompileShader(const ShaderDesc& desc);
    std::shared_ptr<Program> CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders);

    std::shared_ptr<Resource> GetBackBuffer();
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

    std::shared_ptr<Instance> m_instance;
    std::shared_ptr<Adapter> m_adapter;
    std::shared_ptr<Device> m_device;
    std::shared_ptr<Swapchain> m_swapchain;
    std::shared_ptr<Semaphore> m_image_available_semaphore;
    std::shared_ptr<Semaphore> m_rendering_finished_semaphore;
    std::vector<std::shared_ptr<CommandListBox>> m_command_lists;
    std::shared_ptr<CommandListBox> m_command_list;
    std::shared_ptr<Fence> m_fence;
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

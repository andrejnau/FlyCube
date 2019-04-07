#include <AppBox/AppBox.h>
#include <ProgramRef/PixelShaderPS.h>
#include <ProgramRef/VertexShaderVS.h>
#include <WinConsole/WinConsole.h>

class Scene : public IScene
{
public:
    Scene(Context& context, int width, int height)
        : m_context(context)
        , m_width(width)
        , m_height(height)
        , m_program(context)
    {
        std::vector<uint32_t> indices = { 0, 1, 2 };
        m_indices = m_context.CreateBuffer(BindFlag::kIbv, sizeof(uint32_t) * indices.size(), sizeof(uint32_t));
        m_context.UpdateSubresource(m_indices, 0, indices.data(), 0, 0);
        std::vector<glm::vec3> positions = {
            glm::vec3(-0.5, -0.5, 0.0),
            glm::vec3(0.0, 0.5, 0.0),
            glm::vec3(0.5, -0.5, 0.0)
        };
        m_positions = m_context.CreateBuffer(BindFlag::kVbv, sizeof(glm::vec3) * positions.size(), sizeof(glm::vec3));
        m_context.UpdateSubresource(m_positions, 0, positions.data(), 0, 0);
    }

    virtual void OnRender() override
    {
        m_context.SetViewport(m_width, m_height);
        m_program.UseProgram();
        m_program.ps.om.rtv0.Attach(m_context.GetBackBuffer()).Clear({ 0.0f, 0.2f, 0.4f, 1.0f });
        m_context.IASetIndexBuffer(m_indices, gli::format::FORMAT_R32_UINT_PACK32);
        m_context.IASetVertexBuffer(m_program.vs.ia.POSITION, m_positions);
        m_context.DrawIndexed(3, 0, 0);
        m_context.Present();
    }

private:
    Context& m_context;
    int m_width;
    int m_height;
    Program<PixelShaderPS, VertexShaderVS> m_program;
    Resource::Ptr m_indices;
    Resource::Ptr m_positions;
};

int main(int argc, char *argv[])
{
    WinConsole cmd;
    auto creater = [](Context& context, int width, int height)
    {
        return std::make_unique<Scene>(context, width, height);
    };
    ApiType type = ApiType::kDX12;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == "--dx11")
            type = ApiType::kDX11;
        else if (arg == "--dx12")
            type = ApiType::kDX12;
        else if (arg == "--vk")
            type = ApiType::kVulkan;
        else if (arg == "--gl")
            type = ApiType::kOpenGL;
    }

    std::string title;
    switch (type)
    {
    case ApiType::kDX11:
        title = "[DX11] Triangle";
        break;
    case ApiType::kDX12:
        title = "[DX12] Triangle";
        break;
    case ApiType::kVulkan:
        title = "[Vulkan] Triangle";
        break;
    case ApiType::kOpenGL:
        title = "[OpenGL] Triangle";
        break;
    }

    auto monitor_desc = AppBox::GetPrimaryMonitorDesc();
    return AppBox(creater, type, title, monitor_desc.width / 1.5, monitor_desc.height / 1.5).Run();
}

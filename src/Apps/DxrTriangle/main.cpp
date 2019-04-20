#include <AppBox/AppBox.h>
#include <ProgramRef/RayTracing.h>
#include <ProgramRef/GraphicsVS.h>
#include <ProgramRef/GraphicsPS.h>
#include <WinConsole/WinConsole.h>
#include <Context/DX12Context.h>
#include <Geometry/Geometry.h>
#include <Utilities/State.h>
#include <glm/gtx/transform.hpp>

class Scene : public IScene
{
public:
    Scene(Context& context, int width, int height)
        : m_context(context)
        , m_width(width)
        , m_height(height)
        , m_raytracing_program(context)
        , m_graphics_program(context)
        , m_square(m_context, "model/square.obj")
    {
        ASSERT(context.IsDxrSupported());

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

        m_bottom = m_context.CreateBottomLevelAS({ m_positions, gli::format::FORMAT_RGB32_SFLOAT_PACK32, 3 });

        std::vector<std::pair<Resource::Ptr, glm::mat4>> geometry = {
            { m_bottom, glm::mat4() },
        };
        m_top = m_context.CreateTopLevelAS(geometry);

        m_uav = m_context.CreateTexture(BindFlag::kUav, gli::format::FORMAT_RGBA8_UNORM_PACK8, 1, m_width, m_height);
    }

    virtual void OnRender() override
    {
        m_raytracing_program.UseProgram();
        m_raytracing_program.lib.srv.geometry.Attach(m_top);
        m_raytracing_program.lib.uav.result.Attach(m_uav);
        m_context.DispatchRays(m_width, m_height, 1);

        m_graphics_program.UseProgram();
        m_context.SetViewport(m_width, m_height);
        m_graphics_program.ps.om.rtv0.Attach(m_context.GetBackBuffer());
        m_square.ia.indices.Bind();
        m_square.ia.positions.BindToSlot(m_graphics_program.vs.ia.POSITION);
        m_square.ia.texcoords.BindToSlot(m_graphics_program.vs.ia.TEXCOORD);

        for (auto& range : m_square.ia.ranges)
        {
            m_graphics_program.ps.srv.tex.Attach(m_uav);
            m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }

        m_context.Present();
    }

private:
    Context& m_context;
    int m_width;
    int m_height;
    Program<RayTracing> m_raytracing_program;
    Program<GraphicsVS, GraphicsPS> m_graphics_program;
    Model m_square;
    Resource::Ptr m_indices;
    Resource::Ptr m_positions;
    Resource::Ptr m_bottom;
    Resource::Ptr m_top;
    Resource::Ptr m_uav;
};

int main(int argc, char *argv[])
{
    WinConsole cmd;
    CurState::Instance().DXIL = true;
    auto creater = [](Context& context, int width, int height)
    {
        return std::make_unique<Scene>(context, width, height);
    };
    return AppBox(argc, argv, creater, "Triangle").Run();
}

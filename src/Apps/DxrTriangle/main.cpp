#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <ProgramRef/RayTracing.h>
#include <ProgramRef/GraphicsVS.h>
#include <ProgramRef/GraphicsPS.h>
#include <Geometry/Geometry.h>
#include <glm/gtx/transform.hpp>
#include <stdexcept>

int main(int argc, char *argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("DxrTriangle", settings);
    AppRect rect = app.GetAppRect();
    Context context(settings, app.GetWindow());
    if (!context.IsDxrSupported())
        throw std::runtime_error("Ray Tracing is not supported");

    ProgramHolder<RayTracing> raytracing_program(context);
    ProgramHolder<GraphicsVS, GraphicsPS> graphics_program(context);
    Model square(context, "model/square.obj");

    std::vector<glm::vec3> positions_data = {
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3(0.0, 0.5, 0.0),
        glm::vec3(0.5, -0.5, 0.0)
    };
    std::shared_ptr<Resource> positions = context.CreateBuffer(BindFlag::kVertexBuffer, sizeof(glm::vec3) * positions_data.size());
    context.GetCommandList().UpdateSubresource(positions, 0, positions_data.data(), 0, 0);

    std::shared_ptr<Resource> bottom = context.GetCommandList().CreateBottomLevelAS({ positions, gli::format::FORMAT_RGB32_SFLOAT_PACK32, 3 });
    std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4>> geometry = {
        { bottom, glm::mat4() },
    };
    std::shared_ptr<Resource> top = context.GetCommandList().CreateTopLevelAS(geometry);
    std::shared_ptr<Resource> uav = context.CreateTexture(BindFlag::kUnorderedAccess | BindFlag::kShaderResource, gli::format::FORMAT_RGBA8_UNORM_PACK8, 1, rect.width, rect.height);

    while (!app.PollEvents())
    {
        context.GetCommandList().UseProgram(raytracing_program);
        context.GetCommandList().Attach(raytracing_program.lib.srv.geometry, top);
        context.GetCommandList().Attach(raytracing_program.lib.uav.result, uav);
        context.GetCommandList().DispatchRays(rect.width, rect.height, 1);

        context.GetCommandList().UseProgram(graphics_program);
        context.GetCommandList().SetViewport(rect.width, rect.height);
        context.GetCommandList().Attach(graphics_program.ps.om.rtv0, context.GetBackBuffer());
        square.ia.indices.Bind();
        square.ia.positions.BindToSlot(graphics_program.vs.ia.POSITION);
        square.ia.texcoords.BindToSlot(graphics_program.vs.ia.TEXCOORD);
        for (auto& range : square.ia.ranges)
        {
            context.GetCommandList().Attach(graphics_program.ps.srv.tex, uav);
            context.GetCommandList().DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }

        context.Present();
        app.UpdateFps();
    }
    _exit(0);
}

#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <ProgramRef/RayTracing.h>
#include <ProgramRef/GraphicsVS.h>
#include <ProgramRef/GraphicsPS.h>
#include <Geometry/Geometry.h>
#include <glm/gtx/transform.hpp>

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

    std::vector<uint32_t> indices_data = { 0, 1, 2 };
    std::shared_ptr<Resource> indices = context.CreateBuffer(BindFlag::kIndexBuffer, sizeof(uint32_t) * indices_data.size());
    context.UpdateSubresource(indices, 0, indices_data.data(), 0, 0);
    std::vector<glm::vec3> positions_data = {
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3(0.0, 0.5, 0.0),
        glm::vec3(0.5, -0.5, 0.0)
    };
    std::shared_ptr<Resource> positions = context.CreateBuffer(BindFlag::kVertexBuffer, sizeof(glm::vec3) * positions_data.size());
    context.UpdateSubresource(positions, 0, positions_data.data(), 0, 0);

    std::shared_ptr<Resource> bottom = context.CreateBottomLevelAS({ positions, gli::format::FORMAT_RGB32_SFLOAT_PACK32, 3 });
    std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4>> geometry = {
        { bottom, glm::mat4() },
    };
    std::shared_ptr<Resource> top = context.CreateTopLevelAS(geometry);
    std::shared_ptr<Resource> uav = context.CreateTexture(BindFlag::kUnorderedAccess | BindFlag::kShaderResource, gli::format::FORMAT_RGBA8_UNORM_PACK8, 1, rect.width, rect.height);

    while (!app.PollEvents())
    {
        context.UseProgram(raytracing_program);
        raytracing_program.lib.srv.geometry.Attach(top);
        raytracing_program.lib.uav.result.Attach(uav);
        context.DispatchRays(rect.width, rect.height, 1);

        context.UseProgram(graphics_program);
        context.SetViewport(rect.width, rect.height);
        graphics_program.ps.om.rtv0.Attach(context.GetBackBuffer());
        square.ia.indices.Bind();
        square.ia.positions.BindToSlot(graphics_program.vs.ia.POSITION);
        square.ia.texcoords.BindToSlot(graphics_program.vs.ia.TEXCOORD);
        for (auto& range : square.ia.ranges)
        {
            graphics_program.ps.srv.tex.Attach(uav);
            context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }

        context.Present();
        app.UpdateFps();
    }
    _exit(0);
}

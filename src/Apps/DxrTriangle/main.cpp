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

    std::shared_ptr<CommandListBox> upload_command_list = context.CreateCommandList();
    upload_command_list->Open();

    Model square(context, *upload_command_list, "model/square.obj");

    std::vector<glm::vec3> positions_data = {
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3(0.0, 0.5, 0.0),
        glm::vec3(0.5, -0.5, 0.0)
    };
    std::shared_ptr<Resource> positions = context.CreateBuffer(BindFlag::kVertexBuffer, sizeof(glm::vec3) * positions_data.size());
    upload_command_list->UpdateSubresource(positions, 0, positions_data.data(), 0, 0);
    std::shared_ptr<Resource> bottom = upload_command_list->CreateBottomLevelAS({ positions, gli::format::FORMAT_RGB32_SFLOAT_PACK32, 3 });
    std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4>> geometry = {
        { bottom, glm::mat4() },
    };
    std::shared_ptr<Resource> top = upload_command_list->CreateTopLevelAS(geometry);
    std::shared_ptr<Resource> uav = context.CreateTexture(BindFlag::kUnorderedAccess | BindFlag::kShaderResource, gli::format::FORMAT_RGBA8_UNORM_PACK8, 1, rect.width, rect.height);
    upload_command_list->Close();
    context.ExecuteCommandLists({ upload_command_list });

    std::vector<std::shared_ptr<CommandListBox>> command_lists;
    for (uint32_t i = 0; i < Context::FrameCount; ++i)
        command_lists.emplace_back(context.CreateCommandList());

    while (!app.PollEvents())
    {
        decltype(auto) command_list = command_lists[context.GetFrameIndex()];
        command_list->Open();
        command_list->UseProgram(raytracing_program);
        command_list->Attach(raytracing_program.lib.srv.geometry, top);
        command_list->Attach(raytracing_program.lib.uav.result, uav);
        command_list->DispatchRays(rect.width, rect.height, 1);
        command_list->UseProgram(graphics_program);
        command_list->SetViewport(rect.width, rect.height);
        command_list->Attach(graphics_program.ps.om.rtv0, context.GetBackBuffer(context.GetFrameIndex()));
        square.ia.indices.Bind(*command_list);
        square.ia.positions.BindToSlot(*command_list, graphics_program.vs.ia.POSITION);
        square.ia.texcoords.BindToSlot(*command_list, graphics_program.vs.ia.TEXCOORD);
        for (auto& range : square.ia.ranges)
        {
            command_list->Attach(graphics_program.ps.srv.tex, uav);
            command_list->DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }
        command_list->Close();
        context.ExecuteCommandLists({ command_list });
        context.Present();
        app.UpdateFps();
    }
    _exit(0);
}

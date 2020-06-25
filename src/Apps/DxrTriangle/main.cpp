#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <ProgramRef/RayTracing.h>
#include <glm/gtx/transform.hpp>
#include <stdexcept>

int main(int argc, char *argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("DxrTriangle", settings);
    AppRect rect = app.GetAppRect();
    Context context(settings, app.GetWindow());
    Device& device(*context.GetDevice());
    if (!device.IsDxrSupported())
        throw std::runtime_error("Ray Tracing is not supported");

    ProgramHolder<RayTracing> program(device);

    std::shared_ptr<CommandListBox> upload_command_list = context.CreateCommandList();

    std::vector<glm::vec3> positions_data = {
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3(0.0, 0.5, 0.0),
        glm::vec3(0.5, -0.5, 0.0)
    };
    std::shared_ptr<Resource> positions = device.CreateBuffer(BindFlag::kVertexBuffer | BindFlag::kCopyDest, sizeof(glm::vec3) * positions_data.size());
    upload_command_list->UpdateSubresource(positions, 0, positions_data.data(), 0, 0);
    std::shared_ptr<Resource> bottom = upload_command_list->CreateBottomLevelAS({ positions, gli::format::FORMAT_RGB32_SFLOAT_PACK32, 3 });
    std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4>> geometry = {
        { bottom, glm::mat4(1.0) },
    };
    std::shared_ptr<Resource> top = upload_command_list->CreateTopLevelAS(geometry);
    std::shared_ptr<Resource> uav = device.CreateTexture(BindFlag::kUnorderedAccess | BindFlag::kShaderResource | BindFlag::kCopySource,
                                                         context.GetSwapchain()->GetFormat(), 1, rect.width, rect.height);
    upload_command_list->Close();
    context.ExecuteCommandLists({ upload_command_list });

    std::vector<std::shared_ptr<CommandListBox>> command_lists;
    for (uint32_t i = 0; i < Context::FrameCount; ++i)
    {
        decltype(auto) command_list = context.CreateCommandList();
        command_list->UseProgram(program);
        command_list->Attach(program.lib.srv.geometry, top);
        command_list->Attach(program.lib.uav.result, uav);
        command_list->DispatchRays(rect.width, rect.height, 1);
        command_list->CopyTexture(uav, context.GetBackBuffer(i), { { rect.width, rect.height, 1 } });
        command_list->Close();
        command_lists.emplace_back(command_list);
    }

    while (!app.PollEvents())
    {
        context.ExecuteCommandLists({ command_lists[context.GetFrameIndex()] });
        context.Present();
        app.UpdateFps(context.GetGpuName());
    }
    context.WaitIdle();
    return 0;
}

#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <RenderDevice/RenderDevice.h>
#include <ProgramRef/PixelShaderPS.h>
#include <ProgramRef/VertexShaderVS.h>

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("Triangle", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<RenderDevice> device = CreateRenderDevice(settings, app.GetWindow());
    app.SetGpuName(device->GetGpuName());

    std::shared_ptr<RenderCommandList> upload_command_list = device->CreateRenderCommandList();
    std::vector<uint32_t> ibuf = { 0, 1, 2 };
    std::shared_ptr<Resource> index = device->CreateBuffer(BindFlag::kIndexBuffer | BindFlag::kCopyDest, sizeof(uint32_t) * ibuf.size());
    upload_command_list->UpdateSubresource(index, 0, ibuf.data(), 0, 0);
    std::vector<glm::vec3> pbuf = {
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3(0.0,  0.5, 0.0),
        glm::vec3(0.5, -0.5, 0.0)
    };
    std::shared_ptr<Resource> pos = device->CreateBuffer(BindFlag::kVertexBuffer | BindFlag::kCopyDest, sizeof(glm::vec3) * pbuf.size());
    upload_command_list->UpdateSubresource(pos, 0, pbuf.data(), 0, 0);
    upload_command_list->Close();
    device->ExecuteCommandLists({ upload_command_list });

    ProgramHolder<PixelShaderPS, VertexShaderVS> program(*device);
    program.ps.cbuffer.Settings.color = glm::vec4(1, 0, 0, 1);

    std::vector<std::shared_ptr<RenderCommandList>> command_lists;
    for (uint32_t i = 0; i < settings.frame_count; ++i)
    {
        RenderPassBeginDesc render_pass_desc = {};
        render_pass_desc.colors[0].texture = device->GetBackBuffer(i);
        render_pass_desc.colors[0].clear_color = { 0.0f, 0.2f, 0.4f, 1.0f };

        decltype(auto) command_list = device->CreateRenderCommandList();
        command_list->UseProgram(program);
        command_list->Attach(program.ps.cbv.Settings, program.ps.cbuffer.Settings);
        command_list->SetViewport(0, 0, rect.width, rect.height);
        command_list->IASetIndexBuffer(index, gli::format::FORMAT_R32_UINT_PACK32);
        command_list->IASetVertexBuffer(program.vs.ia.POSITION, pos);
        command_list->BeginRenderPass(render_pass_desc);
        command_list->DrawIndexed(3, 1, 0, 0, 0);
        command_list->EndRenderPass();
        command_list->Close();
        command_lists.emplace_back(command_list);
    }

    while (!app.PollEvents())
    {
        device->ExecuteCommandLists({ command_lists[device->GetFrameIndex()] });
        device->Present();
    }
    device->WaitForIdle();
    return 0;
}

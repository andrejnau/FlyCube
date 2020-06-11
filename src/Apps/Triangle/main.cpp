#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <Context/Context.h>
#include <Context/Program.h>
#include <ProgramRef/PixelShaderPS.h>
#include <ProgramRef/VertexShaderVS.h>

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("Triangle", settings);

    Context context(settings, app.GetWindow());
    AppRect rect = app.GetAppRect();
    ProgramHolder<PixelShaderPS, VertexShaderVS> program(context);

    std::vector<uint32_t> ibuf = { 0, 1, 2 };
    std::shared_ptr<Resource> index = context.CreateBuffer(BindFlag::kIndexBuffer, sizeof(uint32_t) * ibuf.size());
    context.GetCommandList().UpdateSubresource(index, 0, ibuf.data(), 0, 0);
    std::vector<glm::vec3> pbuf = {
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3(0.0,  0.5, 0.0),
        glm::vec3(0.5, -0.5, 0.0)
    };
    std::shared_ptr<Resource> pos = context.CreateBuffer(BindFlag::kVertexBuffer, sizeof(glm::vec3) * pbuf.size());
    context.GetCommandList().UpdateSubresource(pos, 0, pbuf.data(), 0, 0);

    program.ps.cbuffer.Settings.color = glm::vec4(1, 0, 0, 1);

    while (!app.PollEvents())
    {
        context.GetCommandList().UseProgram(program);
        context.GetCommandList().Attach(program.ps.cbv.Settings, program.ps.cbuffer.Settings);
        context.GetCommandList().SetViewport(rect.width, rect.height);
        context.GetCommandList().Attach(program.ps.om.rtv0, context.GetBackBuffer());
        context.GetCommandList().ClearColor(program.ps.om.rtv0, { 0.0f, 0.2f, 0.4f, 1.0f });
        context.GetCommandList().IASetIndexBuffer(index, gli::format::FORMAT_R32_UINT_PACK32);
        context.GetCommandList().IASetVertexBuffer(program.vs.ia.POSITION, pos);
        context.GetCommandList().DrawIndexed(3, 0, 0);
        context.Present();
        app.UpdateFps();
    }
    _exit(0);
}

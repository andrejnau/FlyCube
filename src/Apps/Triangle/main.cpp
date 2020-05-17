#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <Context/Context.h>
#include <ProgramRef/PixelShaderPS.h>
#include <ProgramRef/VertexShaderVS.h>

int main(int argc, char* argv[])
{
    Settings setting = { ApiType::kDX12 };
    ParseArgs(argc, argv, setting);
    AppBox app("Triangle", setting);

    Context context(setting.api_type, app.GetWindow());
    AppRect rect = app.GetAppRect();
    ProgramHolder<PixelShaderPS, VertexShaderVS> program(context);

    std::vector<uint32_t> ibuf = { 0, 1, 2 };
    std::shared_ptr<Resource> index = context.CreateBuffer(BindFlag::kIbv, sizeof(uint32_t) * ibuf.size(), sizeof(uint32_t));
    context.UpdateSubresource(index, 0, ibuf.data(), 0, 0);
    std::vector<glm::vec3> pbuf = {
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3( 0.0,  0.5, 0.0),
        glm::vec3( 0.5, -0.5, 0.0)
    };
    std::shared_ptr<Resource> pos = context.CreateBuffer(BindFlag::kVbv, sizeof(glm::vec3) * pbuf.size(), sizeof(glm::vec3));
    context.UpdateSubresource(pos, 0, pbuf.data(), 0, 0);

    while (!app.PollEvents())
    {
        context.UseProgram(program);
        context.SetViewport(rect.width, rect.height);
        program.ps.om.rtv0.Attach(context.GetBackBuffer()).Clear({ 0.0f, 0.2f, 0.4f, 1.0f });
        context.IASetIndexBuffer(index, gli::format::FORMAT_R32_UINT_PACK32);
        context.IASetVertexBuffer(program.vs.ia.POSITION, pos);
        program.ps.cbuffer.Settings.color = glm::vec4(1, 0, 0, 1);
        context.DrawIndexed(3, 0, 0);
        context.Present();
    }
}

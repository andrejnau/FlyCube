#include <AppBox/AppBox.h>
#include <ProgramRef/PixelShaderPS.h>
#include <ProgramRef/VertexShaderVS.h>

int main(int argc, char* argv[])
{
    AppBox app(argc, argv, "Triangle");
    Context& context = app.GetContext();
    AppRect rect = app.GetAppRect();
    Program<PixelShaderPS, VertexShaderVS> program(context);

    std::vector<uint32_t> ibuf = { 0, 1, 2 };
    Resource::Ptr index = context.CreateBuffer(BindFlag::kIbv, sizeof(uint32_t) * ibuf.size(), sizeof(uint32_t));
    context.UpdateSubresource(index, 0, ibuf.data(), 0, 0);
    std::vector<glm::vec3> pbuf = {
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3( 0.0,  0.5, 0.0),
        glm::vec3( 0.5, -0.5, 0.0)
    };
    Resource::Ptr pos = context.CreateBuffer(BindFlag::kVbv, sizeof(glm::vec3) * pbuf.size(), sizeof(glm::vec3));
    context.UpdateSubresource(pos, 0, pbuf.data(), 0, 0);

    while (!app.ShouldClose())
    {
        program.UseProgram();
        context.SetViewport(rect.width, rect.height);
        program.ps.om.rtv0.Attach(context.GetBackBuffer()).Clear({ 0.0f, 0.2f, 0.4f, 1.0f });
        context.IASetIndexBuffer(index, gli::format::FORMAT_R32_UINT_PACK32);
        context.IASetVertexBuffer(program.vs.ia.POSITION, pos);
        program.ps.cbuffer.Settings.color = glm::vec4(1, 0, 0, 1);
        context.DrawIndexed(3, 0, 0);
        context.Present();
        app.PollEvents();
    }
}

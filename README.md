# FlyCube

C++ graphics API agnostic, simple Engine/Framework, Sponza viewer.

* Supported rendering backends
  * DirectX 11
  * DirectX 12 with DirectX Ray Tracing API
  * Vulkan
  * OpenGL

* Supported platforms
  * Windows 10 only

* Engine Features
  * HLSL as shading language for all backends
    * Compilation in DXBC, DXIL, SPIRV
    * Cross-compilation in GLSL with SPIRV-Cross
  * Generated shader helper by shader reflection
    * Easy to use resources binding
    * Constant buffers proxy for compile time access to members
  * Loading of images & 3D models based on gli, SOIL, assimp

* Scene Features
  * Deferred rendering
  * Physically based rendering
  * Image based lighting
  * Ambient occlusion
    * Raytracing
    * Screen space
  * Normal mapping
  * Point shadow mapping
  * Skeletal animation
  * Multisample anti-aliasing
  * Tone mapping
  * Simple imgui based UI settings

![sponza.png](screenshots/sponza.png)

## Engine API example
```cpp
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
```

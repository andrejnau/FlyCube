#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <Geometry/Geometry.h>
#include <Camera/Camera.h>
#include <Utilities/FormatHelper.h>
#include <ProgramRef/MeshletMS.h>
#include <ProgramRef/MeshletPS.h>
#include <glm/gtx/transform.hpp>
#include <stdexcept>

constexpr uint32_t kMaxOutputPrimitives = 10;

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("MeshShading", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<RenderDevice> device = CreateRenderDevice(settings, app.GetWindow());
    if (!device->IsMeshShadingSupported())
        throw std::runtime_error("Mesh Shading is not supported");
    app.SetGpuName(device->GetGpuName());

    auto dsv = device->CreateTexture(BindFlag::kDepthStencil, gli::format::FORMAT_D32_SFLOAT_PACK32, 1, rect.width, rect.height, 1);
    auto sampler = device->CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever
    });

    Camera camera;
    camera.SetCameraPos(glm::vec3(-3.0, 2.75, 0.0));
    camera.SetCameraYaw(-178.0f);
    camera.SetCameraYaw(-1.75f);
    camera.SetViewport(rect.width, rect.height);

    std::shared_ptr<RenderCommandList> upload_command_list = device->CreateRenderCommandList();

    Model model(*device, *upload_command_list, ASSETS_PATH"model/export3dcoat/export3dcoat.obj");
    model.matrix = glm::scale(glm::vec3(0.1f)) * glm::translate(glm::vec3(0.0f, 30.0f, 0.0f)) * glm::rotate(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    upload_command_list->Close();
    device->ExecuteCommandLists({ upload_command_list });

    ProgramHolder<MeshletMS, MeshletPS> program(*device);
    program.ps.cbuffer.Constants.DrawMeshlets = 1;
    program.ms.cbuffer.Constants.World = glm::transpose(model.matrix);
    program.ms.cbuffer.Constants.WorldView = program.ms.cbuffer.Constants.World * glm::transpose(camera.GetViewMatrix());
    program.ms.cbuffer.Constants.WorldViewProj = program.ms.cbuffer.Constants.WorldView * glm::transpose(camera.GetProjectionMatrix());

    std::vector<std::shared_ptr<RenderCommandList>> command_lists;
    for (uint32_t i = 0; i < settings.frame_count; ++i)
    {
        RenderPassBeginDesc render_pass_desc = {};
        render_pass_desc.colors[0].texture = device->GetBackBuffer(i);
        render_pass_desc.colors[0].clear_color = { 1.0f, 1.0f, 1.0f, 1.0f };
        render_pass_desc.depth_stencil.texture = dsv;
        render_pass_desc.depth_stencil.clear_depth = 1.0;

        decltype(auto) command_list = device->CreateRenderCommandList();
        command_list->UseProgram(program);
        command_list->SetViewport(0, 0, rect.width, rect.height);
        command_list->Attach(program.ps.cbv.Constants, program.ps.cbuffer.Constants);

        command_list->Attach(program.ms.cbv.Constants, program.ms.cbuffer.Constants);
        command_list->Attach(program.ms.cbv.MeshInfo, program.ms.cbuffer.MeshInfo);

        command_list->Attach(program.ms.srv.Position, model.ia.positions.GetBuffer());
        command_list->Attach(program.ms.srv.Normal, model.ia.normals.GetBuffer());
        command_list->Attach(program.ms.srv.VertexIndices, model.ia.indices.GetBuffer());

        command_list->BeginRenderPass(render_pass_desc);
        for (auto& range : model.ia.ranges)
        {
            auto& material = model.GetMaterial(range.id);
            program.ms.cbuffer.MeshInfo.IndexCount = range.index_count;
            program.ms.cbuffer.MeshInfo.IndexOffset = 0;
            command_list->DispatchMesh((range.index_count + kMaxOutputPrimitives - 1)/ kMaxOutputPrimitives);
        }
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

#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <Device/Device.h>
#include <Context/Program.h>
#include <ProgramRef/PixelShaderPS.h>
#include <ProgramRef/VertexShaderVS.h>
#include <Geometry/Geometry.h>
#include <glm/gtx/transform.hpp>
#include <Camera/Camera.h>
#include <Utilities/FormatHelper.h>

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("ModelViewer", settings);

    Context context(settings, app.GetWindow());
    Device& device(*context.GetDevice());
    AppRect rect = app.GetAppRect();
    ProgramHolder<PixelShaderPS, VertexShaderVS> program(device);

    std::shared_ptr<CommandListBox> upload_command_list = context.CreateCommandList();

    Model model(device, *upload_command_list, "model/export3dcoat/export3dcoat.obj");
    model.matrix = glm::scale(glm::vec3(0.1f)) * glm::translate(glm::vec3(0.0f, 30.0f, 0.0f)) * glm::rotate(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    auto dsv = device.CreateTexture(BindFlag::kDepthStencil, gli::format::FORMAT_D32_SFLOAT_PACK32, 1, rect.width, rect.height, 1);
    auto sampler = device.CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever
    });

    Camera camera;
    camera.SetCameraPos(glm::vec3(-3.0, 2.75, 0.0));
    camera.SetCameraYaw(-178.0f);
    camera.SetCameraYaw(-1.75f);
    camera.SetViewport(rect.width, rect.height);

    program.vs.cbuffer.ConstantBuf.model = glm::transpose(model.matrix);
    program.vs.cbuffer.ConstantBuf.view = glm::transpose(camera.GetViewMatrix());
    program.vs.cbuffer.ConstantBuf.projection = glm::transpose(camera.GetProjectionMatrix());
    program.vs.cbuffer.ConstantBuf.normalMatrix = glm::transpose(glm::transpose(glm::inverse(model.matrix)));

    std::shared_ptr<Resource> shading_rate_texture;
    if (device.IsVariableRateShadingSupported())
    {
        std::vector<ShadingRate> shading_rate;
        uint32_t tile_size = device.GetShadingRateImageTileSize();
        uint32_t shading_rate_width = (rect.width + tile_size - 1) / tile_size;
        uint32_t shading_rate_height = (rect.height + tile_size - 1) / tile_size;
        for (uint32_t y = 0; y < shading_rate_height; ++y)
        {
            for (uint32_t x = 0; x < shading_rate_width; ++x)
            {
                if (x > (shading_rate_width / 2))
                    shading_rate.emplace_back(ShadingRate::k4x4);
                else
                    shading_rate.emplace_back(ShadingRate::k1x1);
            }
        }
        shading_rate_texture = device.CreateTexture(BindFlag::kShadingRateSource | BindFlag::kCopyDest, gli::format::FORMAT_R8_UINT_PACK8, 1, shading_rate_width, shading_rate_height);
        size_t num_bytes = 0;
        size_t row_bytes = 0;
        GetFormatInfo(shading_rate_width, shading_rate_height, gli::format::FORMAT_R8_UINT_PACK8, num_bytes, row_bytes);
        upload_command_list->UpdateSubresource(shading_rate_texture, 0, shading_rate.data(), row_bytes, num_bytes);
    }

    upload_command_list->Close();
    context.ExecuteCommandLists({ upload_command_list });

    std::vector<std::shared_ptr<CommandListBox>> command_lists;
    for (uint32_t i = 0; i < Context::FrameCount; ++i)
    {
        decltype(auto) command_list = context.CreateCommandList();
        command_list->UseProgram(program);
        command_list->SetViewport(rect.width, rect.height);
        command_list->Attach(program.ps.om.rtv0, context.GetBackBuffer(i));
        command_list->Attach(program.ps.om.dsv, dsv);
        command_list->Attach(program.vs.cbv.ConstantBuf, program.vs.cbuffer.ConstantBuf);

        command_list->ClearColor(program.ps.om.rtv0, { 1.0f, 1.0f, 1.0f, 1.0f });
        command_list->ClearDepth(program.ps.om.dsv, 1.0f);

        model.ia.indices.Bind(*command_list);
        model.ia.positions.BindToSlot(*command_list, program.vs.ia.POSITION);
        model.ia.normals.BindToSlot(*command_list, program.vs.ia.NORMAL);
        model.ia.texcoords.BindToSlot(*command_list, program.vs.ia.TEXCOORD);
        model.ia.tangents.BindToSlot(*command_list, program.vs.ia.TANGENT);

        if (device.IsVariableRateShadingSupported())
        {
            command_list->RSSetShadingRate(ShadingRate::k1x1, std::array<ShadingRateCombiner, 2>{ ShadingRateCombiner::kPassthrough, ShadingRateCombiner::kOverride });
            command_list->RSSetShadingRateImage(shading_rate_texture);
        }

        for (auto& range : model.ia.ranges)
        {
            auto& material = model.GetMaterial(range.id);
            command_list->Attach(program.ps.srv.albedoMap, material.texture.albedo);
            command_list->DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
        }

        if (device.IsVariableRateShadingSupported())
        {
            command_list->RSSetShadingRateImage({});
            command_list->RSSetShadingRate(ShadingRate::k1x1);
        }

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

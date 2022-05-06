#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <Geometry/Geometry.h>
#include <Camera/Camera.h>
#include <Utilities/FormatHelper.h>
#include <ProgramRef/PixelShader.h>
#include <ProgramRef/VertexShader.h>
#include <glm/gtx/transform.hpp>
#include <stdexcept>

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("VariableRateShading", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<RenderDevice> device = CreateRenderDevice(settings, app.GetNativeWindow(), rect.width, rect.height);
    if (!device->IsVariableRateShadingSupported())
        throw std::runtime_error("Variable rate shading is not supported");
    app.SetGpuName(device->GetGpuName());

    auto dsv = device->CreateTexture(BindFlag::kDepthStencil, gli::format::FORMAT_D32_SFLOAT_PACK32, 1, rect.width, rect.height, 1);
    auto sampler = device->CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever
    });

    ViewDesc sampler_view_desc = {};
    sampler_view_desc.bindless = true;
    sampler_view_desc.view_type = ViewType::kSampler;
    auto sampler_view = device->CreateView(sampler, sampler_view_desc);

    Camera camera;
    camera.SetCameraPos(glm::vec3(-3.0, 2.75, 0.0));
    camera.SetCameraYaw(-178.0f);
    camera.SetCameraYaw(-1.75f);
    camera.SetViewport(rect.width, rect.height);

    std::shared_ptr<RenderCommandList> upload_command_list = device->CreateRenderCommandList();

    Model model(*device, *upload_command_list, ASSETS_PATH"model/export3dcoat/export3dcoat.obj");
    model.matrix = glm::scale(glm::vec3(0.1f)) * glm::translate(glm::vec3(0.0f, 30.0f, 0.0f)) * glm::rotate(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    ProgramHolder<PixelShader, VertexShader> program(*device);
    program.vs.cbuffer.ConstantBuf.model = glm::transpose(model.matrix);
    program.vs.cbuffer.ConstantBuf.view = glm::transpose(camera.GetViewMatrix());
    program.vs.cbuffer.ConstantBuf.projection = glm::transpose(camera.GetProjectionMatrix());
    program.vs.cbuffer.ConstantBuf.normalMatrix = glm::transpose(glm::transpose(glm::inverse(model.matrix)));

    std::vector<ShadingRate> shading_rate;
    uint32_t tile_size = device->GetShadingRateImageTileSize();
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
    std::shared_ptr<Resource> shading_rate_texture = device->CreateTexture(BindFlag::kShadingRateSource | BindFlag::kCopyDest, gli::format::FORMAT_R8_UINT_PACK8, 1, shading_rate_width, shading_rate_height);
    size_t num_bytes = 0;
    size_t row_bytes = 0;
    GetFormatInfo(shading_rate_width, shading_rate_height, gli::format::FORMAT_R8_UINT_PACK8, num_bytes, row_bytes);
    upload_command_list->UpdateSubresource(shading_rate_texture, 0, shading_rate.data(), row_bytes, num_bytes);

    upload_command_list->Close();
    device->ExecuteCommandLists({ upload_command_list });

    ViewDesc shading_rate_view_desc = {};
    shading_rate_view_desc.dimension = ViewDimension::kTexture2D;
    shading_rate_view_desc.view_type = ViewType::kShadingRateSource;
    std::shared_ptr<View> shading_rate_view = device->CreateView(shading_rate_texture, shading_rate_view_desc);

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
        command_list->Attach(program.vs.cbv.ConstantBuf, program.vs.cbuffer.ConstantBuf);
        model.ia.indices.Bind(*command_list);
        model.ia.positions.BindToSlot(*command_list, program.vs.ia.POSITION);
        model.ia.normals.BindToSlot(*command_list, program.vs.ia.NORMAL);
        model.ia.texcoords.BindToSlot(*command_list, program.vs.ia.TEXCOORD);
        model.ia.tangents.BindToSlot(*command_list, program.vs.ia.TANGENT);
        command_list->RSSetShadingRateImage(shading_rate_view);

        command_list->BeginRenderPass(render_pass_desc);
        for (auto& range : model.ia.ranges)
        {
            auto& material = model.GetMaterial(range.id);
            command_list->Attach(program.ps.srv.albedoMap, material.texture.albedo);
            command_list->DrawIndexed(range.index_count, 1, range.start_index_location, range.base_vertex_location, 0);
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

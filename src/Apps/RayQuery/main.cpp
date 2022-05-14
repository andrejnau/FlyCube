#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <Geometry/Geometry.h>
#include <Camera/Camera.h>
#include <Utilities/FormatHelper.h>
#include <glm/gtx/transform.hpp>
#include <stdexcept>

#include "ProgramRef/RayQuery.h"

// https://nvpro-samples.github.io/vk_mini_path_tracer/index.html

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("RayQuery", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<RenderDevice> device = CreateRenderDevice(settings, app.GetNativeWindow(), rect.width, rect.height);
    if (!device->IsRayQuerySupported())
        throw std::runtime_error("Ray Query is not supported");
    app.SetGpuName(device->GetGpuName());

    std::shared_ptr<RenderCommandList> upload_command_list = device->CreateRenderCommandList();

    // UAV
    const static int uavWidth = rect.width;
    const static int uavHeight = rect.height;
    std::shared_ptr<Resource> uav = device->CreateTexture(BindFlag::kUnorderedAccess | BindFlag::kShaderResource | BindFlag::kCopySource,
                                                          device->GetFormat(), 1, uavWidth, uavHeight);

    // Load model
    Model model(*device, *upload_command_list, ASSETS_PATH"model/CornellBox/CornellBox-Original-Merged.obj");

    // build BLAS
    std::shared_ptr<Resource> positions = device->CreateBuffer(BindFlag::kVertexBuffer | BindFlag::kCopyDest, sizeof(glm::vec3) * model.meshes[0].positions.size());
    upload_command_list->UpdateSubresource(positions, 0, model.meshes[0].positions.data(), 0, 0);
    std::shared_ptr<Resource> indices = device->CreateBuffer(BindFlag::kIndexBuffer | BindFlag::kCopyDest, sizeof(uint32_t) * model.meshes[0].indices.size());
    upload_command_list->UpdateSubresource(indices, 0, model.meshes[0].indices.data(), 0, 0);
    RaytracingGeometryDesc raytracing_geometry_desc = {
            { positions, gli::format::FORMAT_RGB32_SFLOAT_PACK32, (uint32_t)model.meshes[0].positions.size(), 0 },
            { indices, gli::format::FORMAT_R32_UINT_PACK32, (uint32_t)model.meshes[0].indices.size(), 0 },
            RaytracingGeometryFlags::kOpaque };
    std::shared_ptr<Resource> bottom = device->CreateBottomLevelAS({ raytracing_geometry_desc }, BuildAccelerationStructureFlags::kNone);
    upload_command_list->BuildBottomLevelAS({}, bottom, { raytracing_geometry_desc }, BuildAccelerationStructureFlags::kNone);

    // build TLAS
    std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4>> geometry = {
            { bottom, glm::mat4(1.0) },
    };
    std::shared_ptr<Resource> top = device->CreateTopLevelAS(geometry.size(), BuildAccelerationStructureFlags::kNone);
    upload_command_list->BuildTopLevelAS({}, top, geometry);

    upload_command_list->Close();
    device->ExecuteCommandLists({ upload_command_list });

    ProgramHolder<RayQuery> program(*device);
    program.cs.cbuffer.resolution.screenWidth = uavWidth;
    program.cs.cbuffer.resolution.screenHeight = uavHeight;

    std::vector<std::shared_ptr<RenderCommandList>> command_lists;
    for (uint32_t i = 0; i < settings.frame_count; ++i)
    {
        decltype(auto) command_list = device->CreateRenderCommandList();
        command_list->BeginEvent("RayQuery Pass");
        command_list->UseProgram(program);
        command_list->Attach(program.cs.cbv.resolution, program.cs.cbuffer.resolution);
        command_list->Attach(program.cs.uav.imageData, uav);
        command_list->Attach(program.cs.srv.tlas, top);
        command_list->Dispatch((uint32_t(uavWidth) + 8 - 1)/8, (uint32_t(uavHeight) + 8 - 1)/8, 1);
        command_list->CopyTexture(uav, device->GetBackBuffer(i), { { (uint32_t)uavWidth, (uint32_t)uavHeight, 1 } });
        command_list->EndEvent();
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

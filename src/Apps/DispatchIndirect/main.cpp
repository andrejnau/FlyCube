#include "AppBox/AppBox.h"
#include "AppBox/ArgsParser.h"
#include "RenderDevice/RenderDevice.h"
#include "Texture/TextureLoader.h"

#include "ProgramRef/Noise.h"
#include "ProgramRef/Task.h"

float GetTime()
{
    using float_seconds = std::chrono::duration<float>;
    static auto start = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    return static_cast<float>(std::chrono::duration_cast<std::chrono::duration<float>>(now - start).count());
}

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("DispatchIndirect", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<RenderDevice> device = CreateRenderDevice(settings, app.GetNativeWindow(), rect.width, rect.height);
    app.SetGpuName(device->GetGpuName());

    std::shared_ptr<RenderCommandList> upload_command_list = device->CreateRenderCommandList();

    const static int uavWidth = 512;
    const static int uavHeight = 512;
    std::shared_ptr<Resource> uav = device->CreateTexture(BindFlag::kUnorderedAccess | BindFlag::kShaderResource | BindFlag::kCopySource,
                                                          device->GetFormat(), 1, uavWidth, uavHeight);

    std::shared_ptr<Resource> argument_buffer = device->CreateBuffer(BindFlag::kUnorderedAccess | BindFlag::kIndirectBuffer | BindFlag::kCopyDest, sizeof(DrawIndexedIndirectCommand));

    upload_command_list->Close();
    device->ExecuteCommandLists({ upload_command_list });

    ProgramHolder<Noise> program(*device);
    ProgramHolder<Task> taskProgram(*device);

    program.cs.cbuffer.computeUniformBlock.time = GetTime();

    std::vector<std::shared_ptr<RenderCommandList>> command_lists;
    for (uint32_t i = 0; i < settings.frame_count; ++i)
    {
        decltype(auto) command_list = device->CreateRenderCommandList();
        command_lists.emplace_back(command_list);
    }

    while (!app.PollEvents())
    {
        program.cs.cbuffer.computeUniformBlock.time = GetTime();

        int32_t frameIndex = device->GetFrameIndex();
        device->Wait(command_lists[frameIndex]->GetFenceValue());
        command_lists[frameIndex]->Reset();

        {
            command_lists[frameIndex]->BeginEvent("Task Pass");
            command_lists[frameIndex]->UseProgram(taskProgram);
            command_lists[frameIndex]->Attach(taskProgram.cs.uav.Argument, argument_buffer);
            command_lists[frameIndex]->Dispatch(1, 1, 1);
            command_lists[frameIndex]->EndEvent();
        }
        {
            command_lists[frameIndex]->BeginEvent("DispatchIndirect Pass");
            command_lists[frameIndex]->UseProgram(program);
            command_lists[frameIndex]->Attach(program.cs.cbv.computeUniformBlock, program.cs.cbuffer.computeUniformBlock);
            command_lists[frameIndex]->Attach(program.cs.uav.Tex0, uav);
            command_lists[frameIndex]->DispatchIndirect(argument_buffer, 0);
            command_lists[frameIndex]->CopyTexture(uav, device->GetBackBuffer(frameIndex), { { uavWidth, uavHeight, 1 } });
            command_lists[frameIndex]->EndEvent();
        }

        command_lists[frameIndex]->Close();

        device->ExecuteCommandLists({ command_lists[frameIndex] });
        device->Present();
    }
    device->WaitForIdle();
    return 0;
}
